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

## Implementation and Usage

The simulator executable is called `my-wrench-simulator`, and invoking it from the command-line without any argument will  display usage information (use `./my-wrench-simulator --help` to see full usage). The simulator implements two options for specifying the simulated hardware platform: (i) via an XML platform file passed as a `--platform_file` command-line argument; or (ii) programmatically.  If the `--platform` command-line argument is passed and given a path to a platform XML file (e.g., `--platform_file ../data/platform.xml`) then the platform will be based on that XML file. Otherwise, the platform is defined programmatically (identically to the platform defined in the XML file). 
