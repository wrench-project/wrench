[![Build Status][travis-badge]][travis-link]
[![GitHub Release][release-badge]][release-link]
[![License: LGPL v3][license-badge]](LICENSE.md)
[![Coverage Status][coveralls-badge]][coveralls-link]
[![Codacy Badge][codacy-badge]][codacy-link]
[![SonarCloud Badge][sonarcloud-badge]][sonarcloud-link]

<img src="doc/images/logo-vertical.png" width="100" />

**_Workflow Management System Simulation Workbench_**

WRENCH is an _open-source library_ for developing workflow simulators. WRENCH exposes several high-level simulation 
abstractions to provide the **building blocks** for developing custom simulators.

More information: [WRENCH Project Website](http://wrench-project.org)

## Prerequisites

WRENCH is fully developed in C++. The code follows the C++11 standard, and thus older 
compilers tend to fail the compilation process. Therefore, we strongly recommend
users to satisfy the following requirements:

- **CMake** - version 3.2.3 or higher
  
And, one of the following:
- **g++** - version 5.0 or higher
- **clang** - version 3.6 or higher

## Dependencies

- [SimGrid](http://simgrid.gforge.inria.fr/) - version 3.17 or higher
- [Lemon C++ library](http://lemon.cs.elte.hu/) - version 1.3.1 or higher 
- [PugiXML](http://pugixml.org/) - version 1.8 or higher 
- [JSON for Modern C++](https://github.com/nlohmann/json) - version 2.1.1 or higher 
- [Google Test](https://github.com/google/googletest) - version 1.8 or higher (only required for running test cases)
- [Doxygen](http://www.doxygen.org) - version 1.8 or higher (only required for generating documentation)


## Building From Source

If all dependencies are installed, compiling and installing WRENCH is as simple as running:

```bash
cmake .
make
make install  # try "sudo make install" if you don't have the permission to write
```

## Get in Touch

The main channel to reach the WRENCH team is via the support email: 
[support@wrench-project.org](mailto:support@wrench-project.org?subject=WRENCH Support Contact: Your Topic).

**Bug Report / Feature Request:** our preferred channel to report a bug or request a feature is via  
WRENCH's [Github Issues Track](https://github.com/wrench-project/wrench/issues).


[travis-badge]:     https://travis-ci.org/wrench-project/wrench.svg?branch=master
[travis-link]:      https://travis-ci.org/wrench-project/wrench
[license-badge]:    https://img.shields.io/badge/License-LGPL%20v3-blue.svg
[coveralls-badge]:  https://coveralls.io/repos/github/wrench-project/wrench/badge.svg?branch=master
[coveralls-link]:   https://coveralls.io/github/wrench-project/wrench?branch=master
[release-badge]:    https://img.shields.io/github/release/wrench-project/wrench.svg
[release-link]:     https://github.com/wrench-project/wrench/releases
[codacy-badge]:     https://img.shields.io/codacy/grade/aef324ea84474fff979a8ff19a4e4681.svg
[codacy-link]:      https://www.codacy.com/app/WRENCH/wrench?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=wrench-project/wrench&amp;utm_campaign=Badge_Grade
[sonarcloud-badge]: https://sonarcloud.io/api/badges/measure?key=wrench&metric=ncloc
[sonarcloud-link]:  https://sonarcloud.io/dashboard?id=wrench
