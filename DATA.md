# Data

## Getting the Data


For the sake of convenience, the reordered (clustered) data is made available through the
[Common Index File Format](https://github.com/osirrc/ciff). Data defining the queries used,
the documents within each cluster, and the cluster processing priorities are also provided.
The data can be downloaded from the following links.

| Collection | Configuration | Size | MD5 | Download |
|:-----------|:--------------|-----:|-----|:---------|
| Queries and Clusters | Misc, see README below. | 898MiB (compressed), 900MiB (uncompressed) | -- | [[FigShare]](https://melbourne.figshare.com/ndownloader/files/28283766)
| Gov2 | CIFF export via Anserini w. Porter stemming | 33GiB (uncompressed), 5.2GiB (compressed) | 5b4cccfcbcec80c76b96109e8a467cfc | [[CloudStor]](https://cloudstor.aarnet.edu.au/plus/s/LnlerBmLyLosNRq/download?path="gov2-qkld-bp.ciff.gz")
)
| ClueWeb09B | CIFF export via Anserini w. Porter stemming | 89GiB (uncompressed), 20GiB (compressed) | 1ad70c59e6198bb524afa8ed044ab272 | [[CloudStor]](https://cloudstor.aarnet.edu.au/plus/s/LnlerBmLyLosNRq/download?path="cw09b-qkld-bp.ciff.gz")


The remainder of this README is concerned with the `Queries and Clusters` file described above.
This data can be downloaded from [FigShare](https://melbourne.figshare.com/articles/dataset/Anytime_Ranking_Data/14722455).

## Queries

See the `queries` directory.

Queries are provided in a normalized (stopped and stemmed) format which PISA
accepts. There are three query sets:
 - cw09b-51-200-anserini.txt
 - gov2-701-850-anserini.txt
 - mqt-anserini.txt 

## Shard / Cluster Maps

See the `cluster-maps` directory.

In our work, documents are organized into sets of clusters, where each
cluster contains topically similar documents. We provide the cluster
maps for both `gov2` and `clueweb09b` which were in turn taken from
the work of Dai et al. (CIKM 2016).
In our work, we used the `QKLD-QInit` shardmaps, and the original files 
can be found [here](http://boston.lti.cs.cmu.edu/appendices/CIKM2016-Dai/).

Note, our shard maps differ from the original ones, as we reorder
the documents within each cluster. So, while each document will still
be in its original shard, the order of the documents within each shard
have been permuted.

Each corpus has three types of files associated with it:
 - `random.*` is a globally ordered collection (with random ordering).
 - `bp.*` is a globally ordered collection (with BP ordering).
 - `qkld-bp.*` is a clustered collection where documents inside each cluster are ordered with BP.

If you are only interested in the *anytime* approach described in our paper, you
can ignore the `random` and `bp` baselines. There are no query clusters provided for
either `random` or `bp` orderings, and hence they only support standard top-k querying,
not the newly proposed processing methods.

Each `.shardmap` has the format `doc_id shard_id`, with one entry per line.
For example, the `qkld-bp.shardmap` file for `cw09b` (with 123 shards) looks like:
```
clueweb09-en0000-53-24565 0
clueweb09-en0000-53-24569 0
clueweb09-en0000-53-24572 0
...
clueweb09-enwp01-79-18087 122
clueweb09-enwp01-75-20640 122
clueweb09-enwp01-79-19772 122

```

For each `.shardmap` file, there is also a `.cluster-range` map. This file simply
maps the cumulative sum of documents in each shard, marking the *end point* of
each document range. So, the `qkld-bp.cluster-range` file for `cw09b` looks like:
```
0 353816
1 985526
2 1215198
...
120 49683219
121 50110812
122 50220189
```
In our paper, this file corresponds to the *cluster map vector*, and allows random
access to the start point of each cluster.


## Cluster Selections

See the `cluster-selections` directory.

At query time, you can specify the order in which clusters should be visited by using
the `*_ordered_range` algorithms. To do so, there must be a shard ordering supplied for
every query. These orderings take the following form:

```
701: 64 27 75 49 71
702: 183 71 185 72 104 
...
850: 27 131 72 104 92
```
This example provides a total of 5 clusters for each query.

For both `gov2` and `cw09b`, there are three types of provided clusters:
- `TREC-ltrr-aol-fast.ranking` corresponds to the TREC topics (701-850 for Gov2 and 51-200 for ClueWeb09B) and are derived from the work of Dai et al. (CIKM 2016).
- `TREC-oracle.ranking` corresponds to the TREC topics, and are derived from the exhaustive rankings for those queries (see sec 5.4 in the paper for more details).
- `MQT-oracle.ranking` corresponds to the MQT topics, and are derived from the exhaustive rankings.
