Installing WRENCH                  {#install}
============

@WRENCHUserDoc <div class="doc-type">User Documentation</div> @endWRENCHDoc
@WRENCHDeveloperDoc  <div class="doc-type">Developer Documentation</div> @endWRENCHDoc
@WRENCHInternalDoc  <div class="doc-type">Internal Documentation</div> @endWRENCHDoc

[TOC]

# Prerequisites #                 {#install-prerequisites}
_______

- **CMake** - version 3.2.3 or superior
  
And, one of the following:
- **g++** - version 5 or superior
- **clang** - version 3.6 or superior


## Dependencies ##                  {#install-prerequisites-dependencies}

- [SimGrid](http://simgrid.gforge.inria.fr/) - version 3.17 or superior
- [Lemon C++ library](http://lemon.cs.elte.hu/) - version 1.3.1 or superior 
- [PugiXML](http://pugixml.org/) - version 1.8 or superior 
- [JSON for Modern C++](https://github.com/nlohmann/json) - version 2.1.1 or superior 
- [Google Test](https://github.com/google/googletest) - version 1.8 or superior (only required for running test cases)
- [Doxygen](http://www.doxygen.org) - version 1.8 or superior (only required for generating documentation)


# Source Install #                  {#install-source}
_______


## Building WRENCH ##               {#install-source-build}

You can download the _@WRENCHRelease.tar.gz_ archive from the 
[GitHub releases](https://github.com/wrench-project/wrench/releases) page. 

~~~~~~~~~~~~~{.sh}
tar xf @WRENCHRelease.tar.gz
cd @WRENCHRelease
cmake .
make
make install
~~~~~~~~~~~~~

If you want to stay on the bleeding edge, you should get the latest git version, and recompile it as you would do for an official archive.

~~~~~~~~~~~~~{.sh}
git clone https://github.com/wrench-project/wrench
~~~~~~~~~~~~~


## Existing Compilation Targets ##  {#install-source-targets}

In most cases, compiling and installing WRENCH is enough:

~~~~~~~~~~~~~{.sh}
make
make install # try "sudo make install" if you don't have the permission to write
~~~~~~~~~~~~~

In addition, several compilation targets are provided in WRENCH.

~~~~~~~~~~~~~{.sh}
make doc               # Builds WRENCH documentation
make unit_tests        # Builds WRENCH unit tests
~~~~~~~~~~~~~
 

If you want to see what is really happening, try adding VERBOSE=1 to your compilation requests:

~~~~~~~~~~~~~{.sh}
make VERBOSE=1
~~~~~~~~~~~~~

