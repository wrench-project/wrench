# WRENCH Simulator

This is an example WRENCH simulator generated using `wrench-init`.

## Build from source

Building uses `cmake` and is as follows:

```bash
mkdir build
cd build
cmake ..
make
sudo make install
```

If you have installed SimGrid and/or WRENCH in non-standard locations,  then you should specify `SimGrid_PATH` and/or `WRENCH_PATH` variables. For instance:

```bash
mkdir build
cd build
cmake -DSimGrid_PATH=/my/simgrid/path/ -DWRENCH_PATH=/my/wrench/path/ ..
make
sudo make install 
```