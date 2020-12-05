#! /bin/bash

# Original WRENCH
for i in $(seq 1 32)
do
    ./wrench-example-io-pagecache-multi ${i} 3 4.4 single_host.xml --wrench-full-log
done

# WRENCH with page cache
for i in $(seq 1 32)
do
    ./wrench-example-io-pagecache-multi ${i} 3 4.4 single_host.xml --pagecache
done
