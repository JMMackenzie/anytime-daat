// ANYTIME: This header managers the textual `clusters` which are supplied to PISA

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

#include "tokenizer.hpp"
#include <boost/algorithm/string/join.hpp>
#include "query/queries.hpp"
#include "query/term_processor.hpp"

namespace pisa {

using cluster_queue = std::vector<uint32_t>;

// Given a file of clusters to visit for each query, map them into an associative structure.
// Input format is: 123 : 0 63 22
// --> For query 123, visit cluster 0, then 63, then 22.
std::unordered_map<std::string, cluster_queue> read_selected_clusters(std::string in_file) {
 
    std::unordered_map<std::string, cluster_queue> selected_clusters;
    std::ifstream is(in_file);
    std::string line;
    while(std::getline(is, line)) {
        auto [id, raw_clusters] = split_query_at_colon(line);
        cluster_queue parsed_clusters;
        std::vector<std::string> cluster_ids;
        boost::split(cluster_ids, raw_clusters, boost::is_any_of("\t, ,\v,\f,\r,\n"));

        auto is_empty = [](const std::string& val) { return val.empty(); };
        // remove_if move matching elements to the end, preparing them for erase.
        cluster_ids.erase(std::remove_if(cluster_ids.begin(), cluster_ids.end(), is_empty), cluster_ids.end());
    
        try {
            auto to_int = [](const std::string& val) { return std::stoi(val); };
            std::transform(cluster_ids.begin(), cluster_ids.end(), std::back_inserter(parsed_clusters), to_int);
        } catch (std::invalid_argument& err) {
            spdlog::error("Could not parse cluster identifiers of query `{}`", raw_clusters);
            exit(1);
        }
        std::string qid = id.value();
        selected_clusters.emplace(qid, parsed_clusters);
    }
    return selected_clusters;
}


};

