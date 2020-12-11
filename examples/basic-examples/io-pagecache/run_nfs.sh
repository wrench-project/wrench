#! /bin/bash

# NFS with original WRENCH
for i in $(seq 1 32)
do
    ./wrench-example-io-pagecache-nfs ${i} 3 4.4 two_hosts.xml --wrench-full-log
done

# NFS with page cache
for i in $(seq 1 32)
do
    ./wrench-example-io-pagecache-nfs ${i} 3 4.4 two_hosts.xml --wrench-full-log --pagecache
done
