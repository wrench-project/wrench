Installing WRENCH                  {#install}
============

[TOC]

# Prerequisites #                 {#install-prerequisites}

WRENCH is developed in `C++`. The code follows the C++11 standard, and thus older 
compilers may fail to compile it. Therefore, we strongly recommend
users to satisfy the following requirements:

- **CMake** - version 3.5 or higher
  
And, one of the following:
- **g++** - version 5.4 or higher
- **clang** - version 3.8 or higher

## Required Dependencies ##                  {#install-prerequisites-dependencies}

- [SimGrid](https://simgrid.org/) -- version 3.29
- [PugiXML](http://pugixml.org/) -- version 1.8 or higher
- [JSON for Modern C++](https://github.com/nlohmann/json) -- version 3.9.0 or higher 

(See the troubleshooting section below if encountering difficulties installing dependencies)

## Optional Dependencies ##                  {#install-prerequisites-opt-dependencies}

- [Google Test](https://github.com/google/googletest) -- version 1.8 or higher (only required for running tests)
- [Doxygen](http://www.doxygen.org) -- version 1.8 or higher (only required for generating documentation)
- [Batsched](https://gitlab.inria.fr/batsim/batsched) -- only needed for realistic simulation of compute resources managed by production batch schedulers

# Source Install #                  {#install-source}

## Building WRENCH ##               {#install-source-build}

You can download the _@WRENCHRelease.tar.gz_ archive from the 
[GitHub releases](https://github.com/wrench-project/wrench/releases) page. Once you have
installed dependencies (see above), you can install WRENCH as follows:

~~~~~~~~~~~~~{.sh}
tar xf @WRENCHRelease.tar.gz
cd @WRENCHRelease
cmake .
make
make install # try "sudo make install" if you do not have write privileges
~~~~~~~~~~~~~

If you want to see actual compiler and linker invocations, add VERBOSE=1 to the compilation command:

~~~~~~~~~~~~~{.sh}
make VERBOSE=1
~~~~~~~~~~~~~

To enable the use of Batsched (provided you have installed that package, see above):
~~~~~~~~~~~~~{.sh}
cmake -DENABLE_BATSCHED=on .
~~~~~~~~~~~~~

If you want to stay on the bleeding edge, you should get the latest git version, and recompile it as you would do for an official archive:

~~~~~~~~~~~~~{.sh}
git clone https://github.com/wrench-project/wrench
~~~~~~~~~~~~~

## Compiling and running unit tests ##  {#install-unit-tests}

Building and running the unit tests, which requires Google Test, is done as:

~~~~~~~~~~~~~{.sh}
make unit_tests      
./unit_tests
~~~~~~~~~~~~~
 
## Installation Troubleshooting ##  {#install-troubleshooting}

#### Could NOT find PkgConfig (missing: PKG_CONFIG_EXECUTABLE)
    
 - This error on MacOS is because the `pkg-config` package is not installed
 - Solution: install this package
    - MacPorts: `sudo port install pkg-config`
    - Brew: `sudo brew install pkg-config`

#### Could not find libgfortran when building the SimGrid dependency

  - This is an error that sometimes occurs on MacOS
  - A quick fix is to disable the SMPI feature of SimGrid when configuring it: `cmake -Denable_smpi=off .`

# Docker Containers #             {#install-docker}

WRENCH is also distributed in Docker containers. Please, visit the
[WRENCH Repository on Docker Hub](https://hub.docker.com/r/wrenchproject/wrench/)
to pull WRENCH's Docker images.

The `latest` tag provides a container with the latest 
[WRENCH release](https://github.com/wrench-project/wrench/releases):

~~~~~~~~~~~~~{.sh}
docker pull wrenchproject/wrench 
# or
docker run --rm -it wrenchproject/wrench /bin/bash
~~~~~~~~~~~~~

The `unstable` tag provides a container with the current code in the GitHub's 
`master` branch:

~~~~~~~~~~~~~{.sh}
docker pull wrenchproject/wrench:unstable
# or
docker run --rm -it wrenchproject/wrench:unstable /bin/bash
~~~~~~~~~~~~~ 

Additional tags are available for all WRENCH releases.
