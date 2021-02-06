#! /bin/bash

rm multi/run_time_original.csv
echo "no_pipeline,run_time\n" > multi/run_time_original.csv
# Original WRENCH
for i in $(seq 1 32)
do
    ./wrench-example-io-pagecache-multi ${i} 3 4.4 single_host.xml
done

rm multi/run_time_pagecache.csv
echo "no_pipeline,run_time\n" > multi/run_time_pagecache.csv
# WRENCH with page cache
for i in $(seq 1 32)
do
    ./wrench-example-io-pagecache-multi ${i} 3 4.4 single_host.xml --pagecache
done
