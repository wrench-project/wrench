Installing WRENCH                  {#install}
============

@WRENCHUserDoc <div class="doc-type">User Documentation</div><div class="doc-link">Other: <a href="../developer/install.html">Developer</a> - <a href="../internal/install.html">Internal</a></div> @endWRENCHDoc
@WRENCHDeveloperDoc  <div class="doc-type">Developer Documentation</div><div class="doc-link">Other: <a href="../user/install.html">User</a> - <a href="../internal/install.html">Internal</a></div> @endWRENCHDoc
@WRENCHInternalDoc  <div class="doc-type">Internal Documentation</div><div class="doc-link">Other: <a href="../user/install.html">User</a> -  <a href="../developer/install.html">Developer</a></div> @endWRENCHDoc

[TOC]

# Prerequisites #                 {#install-prerequisites}

WRENCH is developed in C++. The code follows the C++11 standard, and thus older 
compilers may fail to compile it. Therefore, we strongly recommend
users to satisfy the following requirements:

- **CMake** - version 3.2.3 or higher
  
And, one of the following:
- <b>g++</b> - version 5.0 or higher
- <b>clang</b> - version 3.6 or higher


## Dependencies ##                  {#install-prerequisites-dependencies}

- [SimGrid](http://simgrid.gforge.inria.fr/) - version 3.17 or higher
- [Lemon C++ library](http://lemon.cs.elte.hu/) - version 1.3.1 or higher 
- [PugiXML](http://pugixml.org/) - version 1.8 or higher 
- [JSON for Modern C++](https://github.com/nlohmann/json) - version 2.1.1 or higher 
- [Google Test](https://github.com/google/googletest) - version 1.8 or higher (only required for running test cases)
- [Doxygen](http://www.doxygen.org) - version 1.8 or higher (only required for generating documentation)


# Source Install #                  {#install-source}


## Building WRENCH ##               {#install-source-build}

You can download the _@WRENCHRelease.tar.gz_ archive from the 
[GitHub releases](https://github.com/wrench-project/wrench/releases) page and install it as follows:

~~~~~~~~~~~~~{.sh}
tar xf @WRENCHRelease.tar.gz
cd @WRENCHRelease
cmake .
make
make install
~~~~~~~~~~~~~

If you want to stay on the bleeding edge, you should get the latest git version, and recompile it as you would do for an official archive:

~~~~~~~~~~~~~{.sh}
git clone https://github.com/wrench-project/wrench
~~~~~~~~~~~~~


## Existing Compilation Targets ##  {#install-source-targets}

In most cases, compiling and installing WRENCH is enough:

~~~~~~~~~~~~~{.sh}
make
make install # try "sudo make install" if you don't have the permission to write
~~~~~~~~~~~~~

In addition, several compilation targets are provided in WRENCH:

~~~~~~~~~~~~~{.sh}
make doc               # Builds WRENCH documentation
make unit_tests        # Builds WRENCH unit tests
~~~~~~~~~~~~~
 

If you want to see actual compiler and linker invocations, add VERBOSE=1 to your compilation command:

~~~~~~~~~~~~~{.sh}
make VERBOSE=1
~~~~~~~~~~~~~

