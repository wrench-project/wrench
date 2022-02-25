#! /bin/bash

# nighres pipeline with original WRENCH
./wrench-example-io-pagecache-nighres single_host.xml nighres_workflow.json

# nighres pipeline with page cache
./wrench-example-io-pagecache-nighres single_host.xml nighres_workflow.json --wrench-pagecache-simulation
