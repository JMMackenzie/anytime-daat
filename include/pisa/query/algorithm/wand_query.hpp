#pragma once

#include <vector>

#include "clusters.hpp"
#include "query/queries.hpp"
#include "topk_queue.hpp"

namespace pisa {

struct wand_query {
    explicit wand_query(topk_queue& topk, cluster_map& range_to_docid) : m_topk(topk), m_range_to_docid(range_to_docid) {}

    template <typename CursorRange>
    void operator()(CursorRange&& cursors, uint64_t max_docid)
    {
        using Cursor = typename std::decay_t<CursorRange>::value_type;
        if (cursors.empty()) {
            return;
        }

        std::vector<Cursor*> ordered_cursors;
        ordered_cursors.reserve(cursors.size());
        for (auto& en: cursors) {
            ordered_cursors.push_back(&en);
        }

        auto sort_enums = [&]() {
            // sort enumerators by increasing docid
            std::sort(ordered_cursors.begin(), ordered_cursors.end(), [](Cursor* lhs, Cursor* rhs) {
                return lhs->docid() < rhs->docid();
            });
        };

        sort_enums();
        while (true) {
            // find pivot
            float upper_bound = 0;
            size_t pivot;
            bool found_pivot = false;
            for (pivot = 0; pivot < ordered_cursors.size(); ++pivot) {
                if (ordered_cursors[pivot]->docid() >= max_docid) {
                    break;
                }
                upper_bound += ordered_cursors[pivot]->max_score();
                if (m_topk.would_enter(upper_bound)) {
                    found_pivot = true;
                    break;
                }
            }

            // no pivot found, we can stop the search
            if (!found_pivot) {
                break;
            }

            // check if pivot is a possible match
            uint64_t pivot_id = ordered_cursors[pivot]->docid();
            if (pivot_id == ordered_cursors[0]->docid()) {
                float score = 0;
                for (Cursor* en: ordered_cursors) {
                    if (en->docid() != pivot_id) {
                        break;
                    }
                    score += en->score();
                    en->next();
                }

                m_topk.insert(score, pivot_id);
                // resort by docid
                sort_enums();
            } else {
                // no match, move farthest list up to the pivot
                uint64_t next_list = pivot;
                for (; ordered_cursors[next_list]->docid() == pivot_id; --next_list) {
                }
                ordered_cursors[next_list]->next_geq(pivot_id);
                // bubble down the advanced list
                for (size_t i = next_list + 1; i < ordered_cursors.size(); ++i) {
                    if (ordered_cursors[i]->docid() < ordered_cursors[i - 1]->docid()) {
                        std::swap(ordered_cursors[i], ordered_cursors[i - 1]);
                    } else {
                        break;
                    }
                }
            }
        }
    }

    // ANYTIME: Ordered Range Query
    // This query visits a series of clusters (ranges) in a specified order.
    // It will terminate when it exhausts the list of clusters provided, or
    // when max_clusters have been examined.
    template <typename CursorRange>
    void ordered_range_query(CursorRange&& cursors, const cluster_queue& selected_ranges, const size_t max_clusters)
    {
        using Cursor = typename std::decay_t<CursorRange>::value_type;
        if (cursors.empty()) {
            return;
        }

        // Prepare cursors
        std::vector<Cursor*> ordered_cursors;
        ordered_cursors.reserve(cursors.size());
        for (auto& en: cursors) {
            ordered_cursors.push_back(&en);
        }

        size_t processed_clusters = 0;

        // Main loop operates over the queue of clusters
        for (const auto& shard_id : selected_ranges) {

            // Termination check
            if (processed_clusters == max_clusters) {
                return;
            }
            ++processed_clusters;

            // Pick up the [start, end] range
            auto start = m_range_to_docid[shard_id].first;
            auto end = m_range_to_docid[shard_id].second;

            // Get pivots to the right doc and sets up
            // the range-wise bound scores
            float range_max_score = 0;
            for (auto& en: cursors) {
                en.global_geq(start);
                en.update_range_max_score(shard_id);
                range_max_score += en.max_score();
            }

            // Skip ranges that are dead
            if (!m_topk.would_enter(range_max_score)) {
                continue;
            }


            auto sort_enums = [&]() {
                // sort enumerators by increasing docid
                std::sort(ordered_cursors.begin(), ordered_cursors.end(), [](Cursor* lhs, Cursor* rhs) {
                    return lhs->docid() < rhs->docid();
                });
            };

            // Conduct WAND over the range
            sort_enums();
            while (true) {
  
                // find pivot
                float upper_bound = 0;
                size_t pivot;
                bool found_pivot = false;
                for (pivot = 0; pivot < ordered_cursors.size(); ++pivot) {
                    if (ordered_cursors[pivot]->docid() >= end) {
                        break;
                    }
                    upper_bound += ordered_cursors[pivot]->max_score();
                    if (m_topk.would_enter(upper_bound)) {
                        found_pivot = true;
                        break;
                    }
                }

                // no pivot found, we can stop the search
                if (!found_pivot) {
                    break;
                }

                // check if pivot is a possible match
                uint64_t pivot_id = ordered_cursors[pivot]->docid();
                if (pivot_id == ordered_cursors[0]->docid()) {
 
                    float score = 0;
                    for (Cursor* en: ordered_cursors) {
                        if (en->docid() != pivot_id) {
                            break;
                        }
                        score += en->score();
                        en->next();
 
                    }

                    m_topk.insert(score, pivot_id);
                    // resort by docid
                    sort_enums();
                } else {
                    // no match, move farthest list up to the pivot
                    uint64_t next_list = pivot;
                    for (; ordered_cursors[next_list]->docid() == pivot_id; --next_list) {
                    }
                    ordered_cursors[next_list]->next_geq(pivot_id);
                    // bubble down the advanced list
                    for (size_t i = next_list + 1; i < ordered_cursors.size(); ++i) {
                        if (ordered_cursors[i]->docid() < ordered_cursors[i - 1]->docid()) {
                            std::swap(ordered_cursors[i], ordered_cursors[i - 1]);
                        } else {
                            break;
                        }
                    }
                }
            }
        }
    }
 

    // ANYTIME: BoundSum Range Query
    // This query visits a series of clusters (ranges) based on the BoundSum heuristic.
    // It will terminate when the range-wise upper-bound is lower than the top-k heap
    // threshold, or when max_clusters have been examined.
    template <typename CursorRange>
    void boundsum_range_query(CursorRange&& cursors, const size_t max_clusters)
    {

        using Cursor = typename std::decay_t<CursorRange>::value_type;
        if (cursors.empty()) {
            return;
        }

        // Prepare cursors
        std::vector<Cursor*> ordered_cursors;
        ordered_cursors.reserve(cursors.size());
        for (auto& en: cursors) {
            ordered_cursors.push_back(&en);
        }

        // BoundSum computation: For each range, get the boundsum.
        std::vector<std::pair<size_t, float>> range_and_score;
        size_t range_id = 0;
        for (const auto& range : m_range_to_docid) {
            float range_bound_sum = 0.0f;
            for (auto& en: cursors) {
                range_bound_sum += en.get_range_max_score(range_id);
            }
            range_and_score.emplace_back(range_id, range_bound_sum);
            ++range_id;
        }
        // Now, sort from high to low based on the BoundSum
        std::sort(range_and_score.begin(), range_and_score.end(), [](auto& l, auto& r){return l.second > r.second; });

        size_t processed_clusters = 0;

        // Main loop operates over the high-to-low threshold ranges
        for (const auto& index : range_and_score) {

            // Termination check: number of clusters processed, and thresholds
            if (processed_clusters == max_clusters || !m_topk.would_enter(index.second)) {
                return;
            }
            ++processed_clusters;

            // Pick up the [start, end] range
            auto start = m_range_to_docid[index.first].first;
            auto end = m_range_to_docid[index.first].second;

            for (auto& en: cursors) {
                en.global_geq(start);
                en.update_range_max_score(index.first);
            }

            // Skip ranges that are dead
            if (!m_topk.would_enter(index.second)) {
                continue;
            }


            auto sort_enums = [&]() {
                // sort enumerators by increasing docid
                std::sort(ordered_cursors.begin(), ordered_cursors.end(), [](Cursor* lhs, Cursor* rhs) {
                    return lhs->docid() < rhs->docid();
                });
            };

            // Conduct WAND over the range
            sort_enums();
            while (true) {
  
                // find pivot
                float upper_bound = 0;
                size_t pivot;
                bool found_pivot = false;
                for (pivot = 0; pivot < ordered_cursors.size(); ++pivot) {
                    if (ordered_cursors[pivot]->docid() >= end) {
                        break;
                    }
                    upper_bound += ordered_cursors[pivot]->max_score();
                    if (m_topk.would_enter(upper_bound)) {
                        found_pivot = true;
                        break;
                    }
                }

                // no pivot found, we can stop the search
                if (!found_pivot) {
                    break;
                }

                // check if pivot is a possible match
                uint64_t pivot_id = ordered_cursors[pivot]->docid();
                if (pivot_id == ordered_cursors[0]->docid()) {
 
                    float score = 0;
                    for (Cursor* en: ordered_cursors) {
                        if (en->docid() != pivot_id) {
                            break;
                        }
                        score += en->score();
                        en->next();
 
                    }

                    m_topk.insert(score, pivot_id);
                    // resort by docid
                    sort_enums();
                } else {
                    // no match, move farthest list up to the pivot
                    uint64_t next_list = pivot;
                    for (; ordered_cursors[next_list]->docid() == pivot_id; --next_list) {
                    }
                    ordered_cursors[next_list]->next_geq(pivot_id);
                    // bubble down the advanced list
                    for (size_t i = next_list + 1; i < ordered_cursors.size(); ++i) {
                        if (ordered_cursors[i]->docid() < ordered_cursors[i - 1]->docid()) {
                            std::swap(ordered_cursors[i], ordered_cursors[i - 1]);
                        } else {
                            break;
                        }
                    }
                }
            }
        }
    }
 

    // ANYTIME: BoundSum Timeout Query
    // This is the same as the BoundSum Range Query, except that it will also terminate
    // if the elapsed_latency + (risk_factor * average_range_latency) is greater than
    // the specified timout_latency.
    template <typename CursorRange>
    void boundsum_timeout_query(CursorRange&& cursors, const size_t timeout_microseconds, const float risk_factor = 1.0f)
    {
 
        using Cursor = typename std::decay_t<CursorRange>::value_type;
        if (cursors.empty()) {
            return;
        }

        // Start the timeout clock
        auto start_time = std::chrono::steady_clock::now();

        // Prepare cursors
        std::vector<Cursor*> ordered_cursors;
        ordered_cursors.reserve(cursors.size());
        for (auto& en: cursors) {
            ordered_cursors.push_back(&en);
        }

        // BoundSum computation: For each range, get the boundsum.
        std::vector<std::pair<size_t, float>> range_and_score;
        size_t range_id = 0;
        for (const auto& range : m_range_to_docid) {
            float range_bound_sum = 0.0f;
            for (auto& en: cursors) {
                range_bound_sum += en.get_range_max_score(range_id);
            }
            range_and_score.emplace_back(range_id, range_bound_sum);
            ++range_id;
        }
        // Now, sort from high to low based on the BoundSum
        std::sort(range_and_score.begin(), range_and_score.end(), [](auto& l, auto& r){return l.second > r.second; });

        size_t processed_clusters = 0;
        float mean_latency = 0.0f;
        size_t elapsed_latency = 0;

        // Main loop operates over the high-to-low threshold ranges
        for (const auto& index : range_and_score) {

            // Termination check: elapsed time plus a risk-weighted average per-range latency > timeout,
            // and range-based thresholds
            if ((elapsed_latency + (risk_factor * mean_latency) > timeout_microseconds) || !m_topk.would_enter(index.second)) {
                return;
            }
            ++processed_clusters;

            // Pick up the [start, end] range
            auto start = m_range_to_docid[index.first].first;
            auto end = m_range_to_docid[index.first].second;

            for (auto& en: cursors) {
                en.global_geq(start);
                en.update_range_max_score(index.first);
            }

            auto sort_enums = [&]() {
                // sort enumerators by increasing docid
                std::sort(ordered_cursors.begin(), ordered_cursors.end(), [](Cursor* lhs, Cursor* rhs) {
                    return lhs->docid() < rhs->docid();
                });
            };

            // Conduct WAND over the range
            sort_enums();
            while (true) {
  
                // find pivot
                float upper_bound = 0;
                size_t pivot;
                bool found_pivot = false;
                for (pivot = 0; pivot < ordered_cursors.size(); ++pivot) {
                    if (ordered_cursors[pivot]->docid() >= end) {
                        break;
                    }
                    upper_bound += ordered_cursors[pivot]->max_score();
                    if (m_topk.would_enter(upper_bound)) {
                        found_pivot = true;
                        break;
                    }
                }

                // no pivot found, we can stop the search
                if (!found_pivot) {
                    break;
                }

                // check if pivot is a possible match
                uint64_t pivot_id = ordered_cursors[pivot]->docid();
                if (pivot_id == ordered_cursors[0]->docid()) {
 
                    float score = 0;
                    for (Cursor* en: ordered_cursors) {
                        if (en->docid() != pivot_id) {
                            break;
                        }
                        score += en->score();
                        en->next();
 
                    }

                    m_topk.insert(score, pivot_id);
                    // resort by docid
                    sort_enums();
                } else {
                    // no match, move farthest list up to the pivot
                    uint64_t next_list = pivot;
                    for (; ordered_cursors[next_list]->docid() == pivot_id; --next_list) {
                    }
                    ordered_cursors[next_list]->next_geq(pivot_id);
                    // bubble down the advanced list
                    for (size_t i = next_list + 1; i < ordered_cursors.size(); ++i) {
                        if (ordered_cursors[i]->docid() < ordered_cursors[i - 1]->docid()) {
                            std::swap(ordered_cursors[i], ordered_cursors[i - 1]);
                        } else {
                            break;
                        }
                    }
                }
            }
            
            // Now, we need to recompute elapsed times, range-processing averages
            auto time_now = std::chrono::steady_clock::now();
            elapsed_latency = std::chrono::duration_cast<std::chrono::microseconds>(time_now - start_time).count();
            mean_latency = float(elapsed_latency) / processed_clusters;
        }
    }




    std::vector<std::pair<float, uint64_t>> const& topk() const { return m_topk.topk(); }

  private:
    topk_queue& m_topk;
    cluster_map& m_range_to_docid;

};

}  // namespace pisa
