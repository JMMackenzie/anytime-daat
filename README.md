# Anytime Ranking for Document-Ordered Indexes

This is the code and data repository for the paper **Anytime Ranking on Document-Ordered Indexes** by Joel Mackenzie, Matthias Petri, and Alistair Moffat. 

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

See [the examples](examples.md) for some concrete examples of these different algorithms.

## Annotations
To make life (an epsilon) easier, the modified aspects of the original PISA code have been annotated
with an `//ANYTIME` comment. Hopefully this makes the modifications easier to track for anyone
interested on extending or improving our work.

## Data

[See the data README here.](DATA.md)

## Indexing

The following example operates on `gov2`, but the same instructions apply to `cw09b`.

Navigate to the root directory containing this project.
Collect and unpack the data.
```

echo "Get the collections"
mkdir -p data/ciff-data/
cd data/ciff-data/
## Download the data from the links above via wget or curl, see DATA.md for links
wget https://cloudstor.aarnet.edu.au/plus/s/LnlerBmLyLosNRq/download?path="gov2-qkld-bp.ciff.gz" -O gov2-qkld-bp.ciff.gz
gunzip gov2-qkld-bp.ciff.gz
cd ../..

echo "Get the queries and clusters"
wget https://melbourne.figshare.com/ndownloader/files/28283766 -O queries-and-clusters.tar.gz
tar xzvf queries-and-clusters.tar.gz 
xz --decompress cluster-maps/gov2/qkld-bp.shardmap.xz
```

Install PISA's `ciff` conversion tool. See the [repo here](https://github.com/pisa-engine/ciff) for more
information.
```
cd data
git clone https://github.com/pisa-engine/ciff pisa-ciff
cd pisa-ciff
cargo build --release
cd ..
```

Convert the `.ciff` blobs into canonical PISA indexes:
```
mkdir pisa-canonical
pisa-ciff/target/release/ciff2pisa --ciff-file ciff-data/gov2-qkld-bp.ciff --output pisa-canonical/gov2
```

Build the PISA indexes
```
mkdir pisa-indexes

echo "Compress the index..."
../build/bin/compress_inverted_index --encoding block_simdbp --collection pisa-canonical/gov2 --output pisa-indexes/gov2.block_simdbp.idx

echo "Build the WAND data with ranges included..."
../build/bin/create_wand_data --collection pisa-canonical/gov2 \
                              --output pisa-indexes/gov2.fixed-40-w-ranges.bm25.bmw \
                              --block-size 40 \
                              --scorer bm25 \
                              --document-clusters cluster-maps/gov2/qkld-bp.shardmap

echo "Building the term lexicon..."
../build/bin/lexicon build pisa-canonical/gov2.terms pisa-indexes/gov2.termlex

echo "Building the document identifier map..."
../build/bin/lexicon build pisa-canonical/gov2.documents pisa-indexes/gov2.doclex
```

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
