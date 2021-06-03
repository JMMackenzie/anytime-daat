# Examples

In the following examples, we demonstrate the different querying modes that are available. In all of the 
examples, the underlying corpus is the `gov2` corpus, using ad-hoc queries `701-850`. The BM25 model is used
to retrieve the top `k=1000` documents.

Example 1: Plain `block_max_wand` processing with PISA.
```
foo@bar:/anytime-daat/build$ ./bin/queries --encoding block_simdbp --index /path-to-indexes/gov2-block_simdbp.invidx --wand /path-to-indexes/gov2-variable-40-w-ranges.bmw -k 1000 --scorer bm25 --algorithm block_max_wand --queries /path-to-queries/gov2-701-850-anserini.txt --terms /path-to-indexes/gov2.termlex 
[2021-06-03 12:01:24.994] [stderr] [info] Loading index from /path-to-indexes/gov2-block_simdbp.invidx
[2021-06-03 12:01:27.723] [stderr] [info] Warming up posting lists
[2021-06-03 12:01:28.565] [stderr] [info] Performing block_simdbp queries
[2021-06-03 12:01:28.565] [stderr] [info] K: 1000
[2021-06-03 12:01:28.565] [stderr] [info] Timeout (microseconds) 0
[2021-06-03 12:01:28.565] [stderr] [info] Risk Factor 1
[2021-06-03 12:01:28.565] [stderr] [info] Maximum Clusters: 0
[2021-06-03 12:01:28.565] [stderr] [info] Query type: block_max_wand
[2021-06-03 12:01:28.565] [stderr] [info] Safe: false
[2021-06-03 12:01:32.094] [stderr] [info] ---- block_simdbp block_max_wand
[2021-06-03 12:01:32.094] [stderr] [info] Mean: 7880.85
[2021-06-03 12:01:32.094] [stderr] [info] 50% quantile: 5709
[2021-06-03 12:01:32.094] [stderr] [info] 90% quantile: 15216
[2021-06-03 12:01:32.094] [stderr] [info] 95% quantile: 23795
[2021-06-03 12:01:32.094] [stderr] [info] 99% quantile: 41670
[2021-06-03 12:01:32.094] [stderr] [info] Num. reruns: 0
{"type": "block_simdbp", "query": "block_max_wand", "avg": 7880.85, "q50": 5709, "q90": 15216, "q95": 23795, "q99": 41670}
```

Example 2: Using `block_max_wand` processing over a set of selected query clusters. The clusters used in this example are
explained in the `README.md` file accompanying the data release. However, the main idea is to provide a file of *ordered cluster identifiers*
which tell the algorithm the order in which to visit each cluster. In this example, we'll visit a maximum of `1000` clusters (and, since there
are only 199 clusters, these results will be rank safe).
```
foo@bar:/anytime-daat/build$ ./bin/queries --encoding block_simdbp --index /path-to-indexes/gov2-block_simdbp.invidx --wand /path-to-indexes/gov2-variable-40-w-ranges.bmw -k 1000 --scorer bm25  --queries /path-to-queries/gov2-701-850-anserini.txt --terms /path-to-indexes/gov2.termlex --algorithm block_max_wand_ordered_range --max-clusters 1000 --query-clusters /path-to-selections/gov2/TREC-ltrr-aol-fast.ranking
[2021-06-03 12:00:59.077] [stderr] [info] Loading index from /path-to-indexes/gov2-block_simdbp.invidx
[2021-06-03 12:01:01.875] [stderr] [info] Warming up posting lists
[2021-06-03 12:01:03.009] [stderr] [info] Read 149 queries worth of selected clusters.
[2021-06-03 12:01:03.009] [stderr] [info] Performing block_simdbp queries
[2021-06-03 12:01:03.009] [stderr] [info] K: 1000
[2021-06-03 12:01:03.009] [stderr] [info] Timeout (microseconds) 0
[2021-06-03 12:01:03.009] [stderr] [info] Risk Factor 1
[2021-06-03 12:01:03.009] [stderr] [info] Maximum Clusters: 1000
[2021-06-03 12:01:03.009] [stderr] [info] Query type: block_max_wand_ordered_range
[2021-06-03 12:01:03.009] [stderr] [info] Safe: false
[2021-06-03 12:01:06.287] [stderr] [info] ---- block_simdbp block_max_wand_ordered_range
[2021-06-03 12:01:06.287] [stderr] [info] Mean: 7328.58
[2021-06-03 12:01:06.287] [stderr] [info] 50% quantile: 5155
[2021-06-03 12:01:06.287] [stderr] [info] 90% quantile: 16082
[2021-06-03 12:01:06.287] [stderr] [info] 95% quantile: 23982
[2021-06-03 12:01:06.287] [stderr] [info] 99% quantile: 32790
[2021-06-03 12:01:06.287] [stderr] [info] Num. reruns: 0
{"type": "block_simdbp", "query": "block_max_wand_ordered_range", "avg": 7328.58, "q50": 5155, "q90": 16082, "q95": 23982, "q99": 32790}
```


Example 3: This is the same as the last example, but now we only visit the first 5 clusters for each query. Note the accelerated query latency, and that
these results may not be rank safe.
```
foo@bar:/anytime-daat/build$ ./bin/queries --encoding block_simdbp --index /path-to-indexes/gov2-block_simdbp.invidx --wand /path-to-indexes/gov2-variable-40-w-ranges.bmw -k 1000 --scorer bm25  --queries /path-to-queries/gov2-701-850-anserini.txt --terms /path-to-indexes/gov2.termlex --algorithm block_max_wand_ordered_range --max-clusters 5 --query-clusters /path-to-selections/gov2/TREC-ltrr-aol-fast.ranking
[2021-06-03 12:01:43.588] [stderr] [info] Loading index from /path-to-indexes/gov2-block_simdbp.invidx
[2021-06-03 12:01:45.959] [stderr] [info] Warming up posting lists
[2021-06-03 12:01:46.776] [stderr] [info] Read 149 queries worth of selected clusters.
[2021-06-03 12:01:46.776] [stderr] [info] Performing block_simdbp queries
[2021-06-03 12:01:46.776] [stderr] [info] K: 1000
[2021-06-03 12:01:46.776] [stderr] [info] Timeout (microseconds) 0
[2021-06-03 12:01:46.776] [stderr] [info] Risk Factor 1
[2021-06-03 12:01:46.776] [stderr] [info] Maximum Clusters: 5
[2021-06-03 12:01:46.776] [stderr] [info] Query type: block_max_wand_ordered_range
[2021-06-03 12:01:46.776] [stderr] [info] Safe: false
[2021-06-03 12:01:47.973] [stderr] [info] ---- block_simdbp block_max_wand_ordered_range
[2021-06-03 12:01:47.973] [stderr] [info] Mean: 2669.47
[2021-06-03 12:01:47.973] [stderr] [info] 50% quantile: 2130
[2021-06-03 12:01:47.973] [stderr] [info] 90% quantile: 4971
[2021-06-03 12:01:47.973] [stderr] [info] 95% quantile: 6909
[2021-06-03 12:01:47.973] [stderr] [info] 99% quantile: 10223
[2021-06-03 12:01:47.973] [stderr] [info] Num. reruns: 0
{"type": "block_simdbp", "query": "block_max_wand_ordered_range", "avg": 2669.47, "q50": 2130, "q90": 4971, "q95": 6909, "q99": 10223}
```

Example 4: Instead of providing an ordering to the algorithm, we compute the `BoundSum` ordering and use that to determine which clusters to visit first.
In this example, we visit a maximum of 50 clusters.
```
foo@bar:/anytime-daat/build$ ./bin/queries --encoding block_simdbp --index /path-to-indexes/gov2-block_simdbp.invidx --wand /path-to-indexes/gov2-variable-40-w-ranges.bmw -k 1000 --scorer bm25  --queries /path-to-queries/gov2-701-850-anserini.txt --terms /path-to-indexes/gov2.termlex --algorithm block_max_wand_boundsum --max-clusters 50 
[2021-06-03 12:03:58.181] [stderr] [info] Loading index from /path-to-indexes/gov2-block_simdbp.invidx
[2021-06-03 12:04:00.562] [stderr] [info] Warming up posting lists
[2021-06-03 12:04:01.429] [stderr] [info] Performing block_simdbp queries
[2021-06-03 12:04:01.429] [stderr] [info] K: 1000
[2021-06-03 12:04:01.429] [stderr] [info] Timeout (microseconds) 0
[2021-06-03 12:04:01.429] [stderr] [info] Risk Factor 1
[2021-06-03 12:04:01.430] [stderr] [info] Maximum Clusters: 50
[2021-06-03 12:04:01.430] [stderr] [info] Query type: block_max_wand_boundsum
[2021-06-03 12:04:01.430] [stderr] [info] Safe: false
[2021-06-03 12:04:03.978] [stderr] [info] ---- block_simdbp block_max_wand_boundsum
[2021-06-03 12:04:03.978] [stderr] [info] Mean: 5694.65
[2021-06-03 12:04:03.978] [stderr] [info] 50% quantile: 3924
[2021-06-03 12:04:03.978] [stderr] [info] 90% quantile: 11252
[2021-06-03 12:04:03.978] [stderr] [info] 95% quantile: 17301
[2021-06-03 12:04:03.978] [stderr] [info] 99% quantile: 28491
[2021-06-03 12:04:03.978] [stderr] [info] Num. reruns: 0
{"type": "block_simdbp", "query": "block_max_wand_boundsum", "avg": 5694.65, "q50": 3924, "q90": 11252, "q95": 17301, "q99": 28491}
```

Example 5: Similar to example 4, we use `BoundSum` ordering. However, we now target termination according to a timeout latency of 10 milliseconds.
```
foo@bar:/anytime-daat/build$ ./bin/queries --encoding block_simdbp --index /path-to-indexes/gov2-block_simdbp.invidx --wand /path-to-indexes/gov2-variable-40-w-ranges.bmw -k 1000 --scorer bm25  --queries /path-to-queries/gov2-701-850-anserini.txt --terms /path-to-indexes/gov2.termlex --algorithm block_max_wand_boundsum_timeout --timeout 10000
[2021-06-03 12:05:42.555] [stderr] [info] Loading index from /path-to-indexes/gov2-block_simdbp.invidx
[2021-06-03 12:05:44.927] [stderr] [info] Warming up posting lists
[2021-06-03 12:05:45.793] [stderr] [info] Performing block_simdbp queries
[2021-06-03 12:05:45.793] [stderr] [info] K: 1000
[2021-06-03 12:05:45.793] [stderr] [info] Timeout (microseconds) 10000
[2021-06-03 12:05:45.793] [stderr] [info] Risk Factor 1
[2021-06-03 12:05:45.793] [stderr] [info] Maximum Clusters: 0
[2021-06-03 12:05:45.793] [stderr] [info] Query type: block_max_wand_boundsum_timeout
[2021-06-03 12:05:45.793] [stderr] [info] Safe: false
[2021-06-03 12:05:48.123] [stderr] [info] ---- block_simdbp block_max_wand_boundsum_timeout
[2021-06-03 12:05:48.123] [stderr] [info] Mean: 5212.42
[2021-06-03 12:05:48.123] [stderr] [info] 50% quantile: 4768
[2021-06-03 12:05:48.123] [stderr] [info] 90% quantile: 9789
[2021-06-03 12:05:48.123] [stderr] [info] 95% quantile: 9928
[2021-06-03 12:05:48.123] [stderr] [info] 99% quantile: 10008
[2021-06-03 12:05:48.123] [stderr] [info] Num. reruns: 0
{"type": "block_simdbp", "query": "block_max_wand_boundsum_timeout", "avg": 5212.42, "q50": 4768, "q90": 9789, "q95": 9928, "q99": 10008}
```

Example 6: Similar to example 5, except we now make the termination more risk-sensitive, ensuring an earlier termination. Note how the latency is reduced,
especially at the tail.
```
foo@bar:/anytime-daat/build$ ./bin/queries --encoding block_simdbp --index /path-to-indexes/gov2-block_simdbp.invidx --wand /path-to-indexes/gov2-variable-40-w-ranges.bmw -k 1000 --scorer bm25  --queries /path-to-queries/gov2-701-850-anserini.txt --terms /path-to-indexes/gov2.termlex --algorithm block_max_wand_boundsum_timeout --timeout 10000 --risk 1.5
[2021-06-03 12:06:23.408] [stderr] [info] Loading index from /path-to-indexes/gov2-block_simdbp.invidx
[2021-06-03 12:06:26.179] [stderr] [info] Warming up posting lists
[2021-06-03 12:06:27.232] [stderr] [info] Performing block_simdbp queries
[2021-06-03 12:06:27.232] [stderr] [info] K: 1000
[2021-06-03 12:06:27.232] [stderr] [info] Timeout (microseconds) 10000
[2021-06-03 12:06:27.232] [stderr] [info] Risk Factor 1.5
[2021-06-03 12:06:27.232] [stderr] [info] Maximum Clusters: 0
[2021-06-03 12:06:27.232] [stderr] [info] Query type: block_max_wand_boundsum_timeout
[2021-06-03 12:06:27.232] [stderr] [info] Safe: false
[2021-06-03 12:06:29.532] [stderr] [info] ---- block_simdbp block_max_wand_boundsum_timeout
[2021-06-03 12:06:29.532] [stderr] [info] Mean: 5135.3
[2021-06-03 12:06:29.532] [stderr] [info] 50% quantile: 4717
[2021-06-03 12:06:29.532] [stderr] [info] 90% quantile: 9656
[2021-06-03 12:06:29.532] [stderr] [info] 95% quantile: 9840
[2021-06-03 12:06:29.532] [stderr] [info] 99% quantile: 9942
[2021-06-03 12:06:29.532] [stderr] [info] Num. reruns: 0
{"type": "block_simdbp", "query": "block_max_wand_boundsum_timeout", "avg": 5135.3, "q50": 4717, "q90": 9656, "q95": 9840, "q99": 9942}

```

