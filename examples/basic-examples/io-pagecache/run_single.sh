#! /bin/bash

# Original WRENCH
./wrench-example-io-pagecache-single 20 28 single_host.xml --wrench-full-log
./wrench-example-io-pagecache-single 50 75 single_host.xml --wrench-full-log
./wrench-example-io-pagecache-single 75 110 single_host.xml --wrench-full-log
./wrench-example-io-pagecache-single 100 155 single_host.xml --wrench-full-log

# WRENCH with page cache
./wrench-example-io-pagecache-single 20 28 single_host.xml --writeback
./wrench-example-io-pagecache-single 50 75 single_host.xml --writeback
./wrench-example-io-pagecache-single 75 110 single_host.xml --writeback
./wrench-example-io-pagecache-single 100 155 single_host.xml --writeback
