#pragma once

#include <algorithm>
#include <numeric>
#include <vector>

#include "clusters.hpp"
#include "query/queries.hpp"
#include "topk_queue.hpp"
#include "util/compiler_attribute.hpp"

namespace pisa {

struct maxscore_query {
    explicit maxscore_query(topk_queue& topk, cluster_map& range_to_docid) : m_topk(topk), m_range_to_docid(range_to_docid) {}

    template <typename Cursors>
    [[nodiscard]] PISA_ALWAYSINLINE auto sorted(Cursors&& cursors)
        -> std::vector<typename std::decay_t<Cursors>::value_type>
    {
        std::vector<std::size_t> term_positions(cursors.size());
        std::iota(term_positions.begin(), term_positions.end(), 0);
        std::sort(term_positions.begin(), term_positions.end(), [&](auto&& lhs, auto&& rhs) {
            return cursors[lhs].max_score() > cursors[rhs].max_score();
        });
        std::vector<typename std::decay_t<Cursors>::value_type> sorted;
        for (auto pos: term_positions) {
            sorted.push_back(std::move(cursors[pos]));
        };
        return sorted;
    }

    template <typename Cursors>
    [[nodiscard]] PISA_ALWAYSINLINE auto calc_upper_bounds(Cursors&& cursors) -> std::vector<float>
    {
        std::vector<float> upper_bounds(cursors.size());
        auto out = upper_bounds.rbegin();
        float bound = 0.0;
        for (auto pos = cursors.rbegin(); pos != cursors.rend(); ++pos) {
            bound += pos->max_score();
            *out++ = bound;
        }
        return upper_bounds;
    }

    template <typename Cursors>
    [[nodiscard]] PISA_ALWAYSINLINE auto min_docid(Cursors&& cursors) -> std::uint32_t
    {
        return std::min_element(
                   cursors.begin(),
                   cursors.end(),
                   [](auto&& lhs, auto&& rhs) { return lhs.docid() < rhs.docid(); })
            ->docid();
    }

    enum class UpdateResult : bool { Continue, ShortCircuit };
    enum class DocumentStatus : bool { Insert, Skip };

    template <typename Cursors>
    PISA_ALWAYSINLINE void run_sorted(Cursors&& cursors, uint64_t max_docid)
    {
        auto upper_bounds = calc_upper_bounds(cursors);
        auto above_threshold = [&](auto score) { return m_topk.would_enter(score); };

        auto first_upper_bound = upper_bounds.end();
        auto first_lookup = cursors.end();
        auto next_docid = min_docid(cursors);

        auto update_non_essential_lists = [&] {
            while (first_lookup != cursors.begin()
                   && !above_threshold(*std::prev(first_upper_bound))) {
                --first_lookup;
                --first_upper_bound;
                if (first_lookup == cursors.begin()) {
                    return UpdateResult::ShortCircuit;
                }
            }
            return UpdateResult::Continue;
        };

        if (update_non_essential_lists() == UpdateResult::ShortCircuit) {
            return;
        }

        float current_score = 0;
        std::uint32_t current_docid = 0;

        while (current_docid < max_docid) {
            auto status = DocumentStatus::Skip;
            while (status == DocumentStatus::Skip) {
                if (PISA_UNLIKELY(next_docid >= max_docid)) {
                    return;
                }

                current_score = 0;
                current_docid = std::exchange(next_docid, max_docid);

                std::for_each(cursors.begin(), first_lookup, [&](auto& cursor) {
                    if (cursor.docid() == current_docid) {
                        current_score += cursor.score();
                        cursor.next();
                    }
                    if (auto docid = cursor.docid(); docid < next_docid) {
                        next_docid = docid;
                    }
                });

                status = DocumentStatus::Insert;
                auto lookup_bound = first_upper_bound;
                for (auto pos = first_lookup; pos != cursors.end(); ++pos, ++lookup_bound) {
                    auto& cursor = *pos;
                    if (not above_threshold(current_score + *lookup_bound)) {
                        status = DocumentStatus::Skip;
                        break;
                    }
                    cursor.next_geq(current_docid);
                    if (cursor.docid() == current_docid) {
                        current_score += cursor.score();
                    }
                }
            }
            if (m_topk.insert(current_score, current_docid)
                && update_non_essential_lists() == UpdateResult::ShortCircuit) {
                return;
            }
        }
    }


    template <typename Cursors>
    void operator()(Cursors&& cursors_, uint64_t max_docid)
    {
        if (cursors_.empty()) {
            return;
        }
        auto cursors = sorted(cursors_);
        run_sorted(cursors, max_docid);
        std::swap(cursors, cursors_);
    }

    // ANYTIME: Ordered Range Query
    // This query visits a series of clusters (ranges) in a specified order.
    // It will terminate when it exhausts the list of clusters provided, or
    // when max_clusters have been examined.
    template <typename CursorRange>
    void ordered_range_query(CursorRange&& cursors_, const cluster_queue& selected_ranges, const size_t max_clusters)
    {
        if (cursors_.empty()) {
            return;
        }

        auto cursors = sorted(cursors_);
 
        std::vector<float> upper_bounds(cursors.size());
        
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

            float range_bound = 0.0f;
            auto out = upper_bounds.rbegin();
            for (auto pos = cursors.rbegin(); pos != cursors.rend(); ++pos) {
                pos->global_geq(start);
                pos->update_range_max_score(shard_id);
                range_bound += pos->max_score();
                *out++ = range_bound;
            }

            // Skip ranges that are dead
            if (!m_topk.would_enter(range_bound)) {
                continue;
            }
 
            auto above_threshold = [&](auto score) { return m_topk.would_enter(score); };

            auto first_upper_bound = upper_bounds.end();
            auto first_lookup = cursors.end();

            auto next_docid = min_docid(cursors);

            auto update_non_essential_lists = [&] {
                while (first_lookup != cursors.begin()
                       && !above_threshold(*std::prev(first_upper_bound))) {
                    --first_lookup;
                    --first_upper_bound;
                    if (first_lookup == cursors.begin()) {
                        return UpdateResult::ShortCircuit;
                    }
                }
                return UpdateResult::Continue;
            };

            if (update_non_essential_lists() == UpdateResult::ShortCircuit) {
                continue;
            }

            float current_score = 0;
            std::uint32_t current_docid = 0;

            while (current_docid < end) {
                auto status = DocumentStatus::Skip;
                while (status == DocumentStatus::Skip) {
                    current_score = 0;
                    if (PISA_UNLIKELY(next_docid >= end)) {
                        current_docid = end;
                        break;
                    }

                    current_docid = std::exchange(next_docid, end);
 
                    std::for_each(cursors.begin(), first_lookup, [&](auto& cursor) {
                        if (cursor.docid() == current_docid) {
                            current_score += cursor.score();
                            cursor.next();
 
                        }
                        if (auto docid = cursor.docid(); docid < next_docid) {
                            next_docid = docid;
                        }
                    });

                    status = DocumentStatus::Insert;
                    auto lookup_bound = first_upper_bound;
                    for (auto pos = first_lookup; pos != cursors.end(); ++pos, ++lookup_bound) {
                        auto& cursor = *pos;
                        if (not above_threshold(current_score + *lookup_bound)) {
                            status = DocumentStatus::Skip;
                            break;
                        }
                        cursor.next_geq(current_docid);
                        if (cursor.docid() == current_docid) {
                            current_score += cursor.score();
 
                        }
                    }
                }
                if (m_topk.insert(current_score, current_docid)
                    && update_non_essential_lists() == UpdateResult::ShortCircuit) {
                    break;
                }
            }
        }
    }

    // ANYTIME: BoundSum Range Query
    // This query visits a series of clusters (ranges) based on the BoundSum heuristic.
    // It will terminate when the range-wise upper-bound is lower than the top-k heap
    // threshold, or when max_clusters have been examined.
    template <typename CursorRange>
    void boundsum_range_query(CursorRange&& cursors_, const size_t max_clusters)
    {

        if (cursors_.empty()) {
            return;
        }

        auto cursors = sorted(cursors_);
 
        std::vector<float> upper_bounds(cursors.size());
        
        // BoundSum computation: For each range, get the boundsum.
        std::vector<std::pair<size_t, float>> range_and_score;
        size_t range_id = 0;
        for (const auto& range : m_range_to_docid) {
            float range_bound_sum = 0.0f;
            for (auto pos = cursors.begin(); pos != cursors.end(); ++pos) {
                range_bound_sum += pos->get_range_max_score(range_id);
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

            float range_bound = 0.0f;
            auto out = upper_bounds.rbegin();
            for (auto pos = cursors.rbegin(); pos != cursors.rend(); ++pos) {
                pos->global_geq(start);
                pos->update_range_max_score(index.first);
                range_bound += pos->max_score();
                *out++ = range_bound;
           }

            auto above_threshold = [&](auto score) { return m_topk.would_enter(score); };

            auto first_upper_bound = upper_bounds.end();
            auto first_lookup = cursors.end();

            auto next_docid = min_docid(cursors);

            auto update_non_essential_lists = [&] {
                while (first_lookup != cursors.begin()
                       && !above_threshold(*std::prev(first_upper_bound))) {
                    --first_lookup;
                    --first_upper_bound;
                    if (first_lookup == cursors.begin()) {
                        return UpdateResult::ShortCircuit;
                    }
                }
                return UpdateResult::Continue;
            };

            if (update_non_essential_lists() == UpdateResult::ShortCircuit) {
                continue;
            }

            float current_score = 0;
            std::uint32_t current_docid = 0;

            while (current_docid < end) {
                auto status = DocumentStatus::Skip;
                while (status == DocumentStatus::Skip) {
                    current_score = 0;
                    if (PISA_UNLIKELY(next_docid >= end)) {
                        current_docid = end;
                        break;
                    }

                    current_docid = std::exchange(next_docid, end);
 
                    std::for_each(cursors.begin(), first_lookup, [&](auto& cursor) {
                        if (cursor.docid() == current_docid) {
                            current_score += cursor.score();
                            cursor.next();
 
                        }
                        if (auto docid = cursor.docid(); docid < next_docid) {
                            next_docid = docid;
                        }
                    });

                    status = DocumentStatus::Insert;
                    auto lookup_bound = first_upper_bound;
                    for (auto pos = first_lookup; pos != cursors.end(); ++pos, ++lookup_bound) {
                        auto& cursor = *pos;
                        if (not above_threshold(current_score + *lookup_bound)) {
                            status = DocumentStatus::Skip;
                            break;
                        }
                        cursor.next_geq(current_docid);
                        if (cursor.docid() == current_docid) {
                            current_score += cursor.score();
 
                        }
                    }
                }
                if (m_topk.insert(current_score, current_docid)
                    && update_non_essential_lists() == UpdateResult::ShortCircuit) {
                    break;
                }
            }
        }
    }



    // ANYTIME: BoundSum Timeout Query
    // This is the same as the BoundSum Range Query, except that it will also terminate
    // if the elapsed_latency + (risk_factor * average_range_latency) is greater than
    // the specified timout_latency.
    template <typename CursorRange>
    void boundsum_timeout_query(CursorRange&& cursors_, const size_t timeout_microseconds, const float risk_factor = 1.0f)
    {
        if (cursors_.empty()) {
            return;
        }

        // Start the timeout clock
        auto start_time = std::chrono::steady_clock::now();

        auto cursors = sorted(cursors_);
 
        std::vector<float> upper_bounds(cursors.size());
        
        // BoundSum computation: For each range, get the boundsum.
        std::vector<std::pair<size_t, float>> range_and_score;
        size_t range_id = 0;
        for (const auto& range : m_range_to_docid) {
            float range_bound_sum = 0.0f;
            for (auto pos = cursors.begin(); pos != cursors.end(); ++pos) {
                range_bound_sum += pos->get_range_max_score(range_id);
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

            float range_bound = 0.0f;
            auto out = upper_bounds.rbegin();
            for (auto pos = cursors.rbegin(); pos != cursors.rend(); ++pos) {
                pos->global_geq(start);
                pos->update_range_max_score(index.first);
                range_bound += pos->max_score();
                *out++ = range_bound;
           }

            auto above_threshold = [&](auto score) { return m_topk.would_enter(score); };

            auto first_upper_bound = upper_bounds.end();
            auto first_lookup = cursors.end();

            auto next_docid = min_docid(cursors);

            auto update_non_essential_lists = [&] {
                while (first_lookup != cursors.begin()
                       && !above_threshold(*std::prev(first_upper_bound))) {
                    --first_lookup;
                    --first_upper_bound;
                    if (first_lookup == cursors.begin()) {
                        return UpdateResult::ShortCircuit;
                    }
                }
                return UpdateResult::Continue;
            };

            if (update_non_essential_lists() == UpdateResult::ShortCircuit) {
                continue;
            }

            float current_score = 0;
            std::uint32_t current_docid = 0;

            while (current_docid < end) {
                auto status = DocumentStatus::Skip;
                while (status == DocumentStatus::Skip) {
                    current_score = 0;
                    if (PISA_UNLIKELY(next_docid >= end)) {
                        current_docid = end;
                        break;
                    }

                    current_docid = std::exchange(next_docid, end);
 
                    std::for_each(cursors.begin(), first_lookup, [&](auto& cursor) {
                        if (cursor.docid() == current_docid) {
                            current_score += cursor.score();
                            cursor.next();
 
                        }
                        if (auto docid = cursor.docid(); docid < next_docid) {
                            next_docid = docid;
                        }
                    });

                    status = DocumentStatus::Insert;
                    auto lookup_bound = first_upper_bound;
                    for (auto pos = first_lookup; pos != cursors.end(); ++pos, ++lookup_bound) {
                        auto& cursor = *pos;
                        if (not above_threshold(current_score + *lookup_bound)) {
                            status = DocumentStatus::Skip;
                            break;
                        }
                        cursor.next_geq(current_docid);
                        if (cursor.docid() == current_docid) {
                            current_score += cursor.score();
 
                        }
                    }
                }
                if (m_topk.insert(current_score, current_docid)
                    && update_non_essential_lists() == UpdateResult::ShortCircuit) {
                    break;
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
