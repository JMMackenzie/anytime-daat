This is the code and data repository for the paper **Anytime Ranking on Document-Ordered Indexes** by Joel Mackenzie, Matthias Petri, and Alistair Moffat. The code and data will be made available soon.

## Reference
If you use our anytime extensions to PISA in your own research, you can cite the following reference:
```
TODO
```

## Changes
The main changes in this version of the PISA engine allow for anytime query processing. The main
idea is to cluster the documents in the inverted index, and traverse the index in order of these
clusters (attempting to visit the most promising clusters first). 

## Algorithms implemented
Each of the following algorithms is implemented in terms of the `wand`, `block_max_wand`, and
`maxscore` index traversal algorithms. Hence, ranges will be visited based on some priority, and
the processing within each range will be handled by the traversal algorithm used.
The particular range ordering algorithms are as follows:

- `*_ordered_range` : This query type visits a series of clusters in a pre-defined order, as
accepted via command line parameters (a file of per-query cluster orderings). Termination will
occur when the specified number of clusters have been visited.

- `*_boundsum` : This query type is the same as the `ordered_range` query, but visits clusters
according to the `BoundSum` heuristic. Again, termination will occur when the specified number
of clusters have been visited, but safe early termination is also possible (check the paper for
more details).

- `*_boundsum_timeout` : This is the true "anytime" querying mode. Clusters are visited in the
order specified by `BoundSum`, but termination occurs based on an internal clock and a provided
timeout in microseconds.

So, if you wanted to use `maxscore` to process within each cluster, and you wanted anytime processing, 
you would use the `maxscore_boundsum_timeout` query type.

## Annotations
To make life (an epsilon) easier, the modified aspects of the original PISA code have been annotated
with an `//ANYTIME` comment. Hopefully this makes the modifications easier to track for anyone
interested on extending or improving our work.


# PISA@9bb4d43

This repo is based on [PISA](https://github.com/pisa-engine/pisa/) at commit `9bb4d43` -- please see the original
repo if you are interested in more details.


## Reference

If you use PISA in your own research, you can cite the following reference:
```
@inproceedings{MSMS2019,
  author    = {Antonio Mallia and Michal Siedlaczek and Joel Mackenzie and Torsten Suel},
  title     = {{PISA:} Performant Indexes and Search for Academia},
  booktitle = {Proceedings of the Open-Source {IR} Replicability Challenge co-located
               with 42nd International {ACM} {SIGIR} Conference on Research and Development
               in Information Retrieval, OSIRRC@SIGIR 2019, Paris, France, July 25,
               2019.},
  pages     = {50--56},
  year      = {2019},
  url       = {http://ceur-ws.org/Vol-2409/docker08.pdf}
}
```
