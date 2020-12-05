The example simulators in this directory are similar to the simulators in the bare-metal-bag-of-task directory, but just 
modified to enavle the use of writeback page cache devices.

There are 3 experimental scenarios implemented. In every experiment, we run a number of 
pipelines, each pipeline consists of 3 sequential tasks, which read an input file, 
do some computations, then write the output to a file. The output of a task is the input of 
the following task.

## 1. Single-threaded
 - How to run: run `wrench-example-io-pagecache-single 1 <input_size_in_gb> <cpu_time_in_sec> 
 single_host.xml --single`. Run with `--pagecache` to enable a writeback page cache.
 - Output log files are exported to the directories: `single/original/` if run 
 without `--pagecache` option, or `single/pagecache` with the option. 
 
## 2. Multi-threaded
 - How to run: run script `run_multi.sh`. You can modify the line 5 to disable/enable 
 the use of page cache as in the single-threaded simulator.
 - Output log files are exported to the directories: `multi/original/` if run 
  without `--pagecache` option, or `multi/pagecache` with the option.
  
## 3. NFS
- How to run: run script `run_nfs.sh`. 
- Output log files are exported to the directories: `nfs/original/` if run 
without `--pagecache` option, or `nfs/pagecache` with the option.
