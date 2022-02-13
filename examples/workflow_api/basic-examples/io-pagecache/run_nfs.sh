#! /bin/bash

# NFS with original WRENCH
rm nfs/run_time_original.csv
echo "no_pipeline,run_time\n" > nfs/run_time_original.csv
for i in $(seq 1 32)
do
    ./wrench-example-io-pagecache-nfs ${i} 3 4.4 two_hosts.xml
done

# NFS with page cache
rm nfs/run_time_pagecache.csv
echo "no_pipeline,run_time\n" > nfs/run_time_pagecache.csv
for i in $(seq 1 32)
do
    ./wrench-example-io-pagecache-nfs ${i} 3 4.4 two_hosts.xml --wrench-pagecache-simulation
done
