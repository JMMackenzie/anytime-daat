#pragma once

#include "boost/variant.hpp"

#include "binary_freq_collection.hpp"
#include "configuration.hpp"
#include "score_opt_partition.hpp"

namespace pisa {

struct FixedBlock {
    uint64_t size;
    explicit FixedBlock(const uint64_t in_size) : size(in_size) {}
};

struct VariableBlock {
    float lambda;
    explicit VariableBlock(const float in_lambda) : lambda(in_lambda) {}
};

using BlockSize = boost::variant<FixedBlock, VariableBlock>;

// ANYTIME
template <typename Scorer>
std::pair<std::vector<uint32_t>, std::vector<float>> static_block_partition(
    binary_freq_collection::sequence const& seq, Scorer scorer, const uint64_t block_size,
    std::unordered_map<uint32_t, uint32_t>& doc_to_range)
{
    std::vector<uint32_t> block_docid;
    std::vector<float> block_max_term_weight;
    std::vector<uint32_t> range_docid;
    std::vector<float> range_max_term_weight;

    // Auxiliary vector
    float max_score = 0;
    float block_max_score = 0;
    size_t current_block = 0;
    size_t i;
    uint32_t current_range = doc_to_range[*(seq.docs.begin())];
    float range_max_score = 0;

    for (i = 0; i < seq.docs.size(); ++i) {
        uint64_t docid = *(seq.docs.begin() + i);
        uint64_t freq = *(seq.freqs.begin() + i);
        float score = scorer(docid, freq);
        max_score = std::max(max_score, score);
        if (i == 0 || (i / block_size) == current_block) {
            block_max_score = std::max(block_max_score, score);
        } else {
            block_docid.push_back(*(seq.docs.begin() + i) - 1);
            block_max_term_weight.push_back(block_max_score);
            current_block++;
            block_max_score = std::max((float)0, score);
        }
        // ANYTIME: Range max score check
        if (doc_to_range[docid] != current_range) {
            range_docid.push_back(current_range);
            range_max_term_weight.push_back(range_max_score);
            range_max_score = std::max((float)0, score);
            current_range = doc_to_range[docid];
        } else {
          range_max_score = std::max(range_max_score, score);
        }
    }
    block_docid.push_back(*(seq.docs.begin() + seq.docs.size() - 1));
    block_max_term_weight.push_back(block_max_score);
    range_docid.push_back(current_range);
    range_max_term_weight.push_back(range_max_score);

    return std::make_tuple(block_docid, block_max_term_weight, range_docid, range_max_term_weight);
}

// ANYTIME
template <typename Scorer>
std::pair<std::vector<uint32_t>, std::vector<float>> variable_block_partition(
    binary_freq_collection const& coll,
    binary_freq_collection::sequence const& seq,
    Scorer scorer,
    std::unordered_map<uint32_t, uint32_t>& doc_to_range,
    const float lambda,
    // Antonio Mallia, Giuseppe Ottaviano, Elia Porciani, Nicola Tonellotto, and Rossano Venturini.
    // 2017. Faster BlockMax WAND with Variable-sized Blocks. In Proc. SIGIR
    double eps1 = 0.01,
    double eps2 = 0.4)
{
    // Auxiliary vector
    using doc_score_t = std::pair<uint64_t, float>;
    std::vector<doc_score_t> doc_score;

    std::transform(
        seq.docs.begin(),
        seq.docs.end(),
        seq.freqs.begin(),
        std::back_inserter(doc_score),
        [&](const uint64_t& doc, const uint64_t& freq) -> doc_score_t {
            return {doc, scorer(doc, freq)};
        });

    // ANYTIME: Get range max scores here
    std::vector<uint32_t> range_docid;
    std::vector<float> range_max_term_weight;
    
    uint32_t current_range = doc_to_range[doc_score[0].first];
    float range_max_score = 0;

    for (size_t i = 0; i < doc_score.size(); ++i) {
        uint64_t docid = doc_score[i].first;
        float score = doc_score[i].second;
        // Range max score check
        if (doc_to_range[docid] != current_range) {
            range_docid.push_back(current_range);
            range_max_term_weight.push_back(range_max_score);
            range_max_score = std::max((float)0, score);
            current_range = doc_to_range[docid];
        } else {
          range_max_score = std::max(range_max_score, score);
        }
    }
    range_docid.push_back(current_range);
    range_max_term_weight.push_back(range_max_score);
    
    auto p = score_opt_partition(doc_score.begin(), 0, doc_score.size(), eps1, eps2, lambda);
    
    return std::make_tuple(p.docids, p.max_values, range_docid, range_max_term_weight);
}

}  // namespace pisa
