Installing WRENCH                  {#install}
============

@WRENCHUserDoc <div class="doc-type">User Documentation</div><div class="doc-link">Other: <a href="../developer/install.html">Developer</a> - <a href="../internal/install.html">Internal</a></div> @endWRENCHDoc
@WRENCHDeveloperDoc  <div class="doc-type">Developer Documentation</div><div class="doc-link">Other: <a href="../user/install.html">User</a> - <a href="../internal/install.html">Internal</a></div> @endWRENCHDoc
@WRENCHInternalDoc  <div class="doc-type">Internal Documentation</div><div class="doc-link">Other: <a href="../user/install.html">User</a> -  <a href="../developer/install.html">Developer</a></div> @endWRENCHDoc

[TOC]





# Prerequisites #                 {#install-prerequisites}

WRENCH is developed in `C++`. The code follows the C++11 standard, and thus older 
compilers may fail to compile it. Therefore, we strongly recommend
users to satisfy the following requirements:

- **CMake** - version 3.2.3 or higher
  
And, one of the following:
- <b>g++</b> - version 5.0 or higher
- <b>clang</b> - version 3.6 or higher


## Dependencies ##                  {#install-prerequisites-dependencies}

- [SimGrid](http://simgrid.gforge.inria.fr/) -- version 3.20 or higher
- [Lemon C++ library](http://lemon.cs.elte.hu/) -- version 1.3.1 or higher 
- [PugiXML](http://pugixml.org/) -- version 1.8 or higher 
- [JSON for Modern C++](https://github.com/nlohmann/json) -- version 2.1.1 or higher 
- [Google Test](https://github.com/google/googletest) -- version 1.8 or higher (only required for running test cases)
- [Doxygen](http://www.doxygen.org) -- version 1.8 or higher (only required for generating documentation)
- [Batsched](https://gitlab.inria.fr/batsim/batsched) -- only needed for realistic simulation of resource managed by production batch schedulers



# Source Install #                  {#install-source}


## Building WRENCH ##               {#install-source-build}

You can download the _@WRENCHRelease.tar.gz_ archive from the 
[GitHub releases](https://github.com/wrench-project/wrench/releases) page and install it as follows:

~~~~~~~~~~~~~{.sh}
tar xf @WRENCHRelease.tar.gz
cd @WRENCHRelease
cmake .
make
make install # try "sudo make install" if you do not have write privileges
~~~~~~~~~~~~~

To enable the use of Batsched (provided you have installed that package, see above):
~~~~~~~~~~~~~{.sh}
cmake -DENABLE_BATSCHED .
~~~~~~~~~~~~~



If you want to stay on the bleeding edge, you should get the latest git version, and recompile it as you would do for an official archive:

~~~~~~~~~~~~~{.sh}
git clone https://github.com/wrench-project/wrench
~~~~~~~~~~~~~




## Existing Compilation Targets ##  {#install-source-targets}

In most cases, compiling and installing WRENCH is enough:

~~~~~~~~~~~~~{.sh}
make
make install # try "sudo make install" if you do not have write privileges
~~~~~~~~~~~~~

In addition, several compilation targets are provided in WRENCH:

~~~~~~~~~~~~~{.sh}
make doc               # Builds WRENCH documentation (in the ./docs directory) - Requires Doxygen
make unit_tests        # Builds WRENCH unit tests  (run them by typing ./unit_tests) - Requires Google Test
~~~~~~~~~~~~~
 

If you want to see actual compiler and linker invocations, add VERBOSE=1 to your compilation command:

~~~~~~~~~~~~~{.sh}
make VERBOSE=1
~~~~~~~~~~~~~


# Docker Containers #             {#install-docker}

WRENCH is also distributed in Docker containers. Please, visit the
[WRENCH's Repository on Docker Hub](https://hub.docker.com/r/wrenchproject/wrench/)
to pull WRENCH's Docker images.

The `latest` tag provides a container with the latest 
[WRENCH's release](https://github.com/wrench-project/wrench/releases):

~~~~~~~~~~~~~{.sh}
docker pull wrenchproject/wrench 
# or
docker run -it wrenchproject/wrench /bin/bash
~~~~~~~~~~~~~

 The `unstable` tag provides a container with the current code in the GitHub's `master` 
branch:

~~~~~~~~~~~~~{.sh}
docker pull wrenchproject/wrench:unstable
# or
docker run -it wrenchproject/wrench:unstable /bin/bash
~~~~~~~~~~~~~ 

Additional tags are available for all WRENCH releases. 


## Installation Troubleshooting ##  {#install-troubleshooting}

##### `Could NOT find PkgConfig (missing: PKG_CONFIG_EXECUTABLE)`
    
 - This error on MacOS is because the `pkg-config` package is not installed
 - Solution: install this package
    - MacPorts: `sudo port install pkg-config`
    - Brew: `sudo brew install pkg-config`