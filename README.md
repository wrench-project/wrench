[![Build Status][travis-badge]][travis-link]
[![GitHub Release][release-badge]][release-link]
[![License: LGPL v3][license-badge]](LICENSE.md)
[![Coverage Status][coveralls-badge]][coveralls-link]
[![Codacy Badge][codacy-badge]][codacy-link]
[![SonarCloud Badge][sonarcloud-badge]][sonarcloud-link]
[![CII Best Practices][cii-badge]][cii-link]

<img src="doc/images/logo-vertical.png" width="100" />

**_Workflow Management System Simulation Workbench_**

WRENCH is an _open-source library_ for developing workflow simulators. WRENCH exposes several high-level simulation 
abstractions to provide the **building blocks** for developing custom simulators.

More information and Documentation: [WRENCH Project Website](http://wrench-project.org)

**WRENCH Research Paper:**
- H. Casanova, S. Pandey, J. Oeth, R. Tanaka, F. Suter, and R. Ferreira da Silva, “[WRENCH: A Framework for Simulating Workflow Management Systems](http://rafaelsilva.com/wp-content/papercite-data/pdf/casanova-works-2018.pdf),” in 13th Workshop on Workflows in Support of Large-Scale Science (WORKS’18), 2018, p. 74–85. 

## Prerequisites

WRENCH is fully developed in C++. The code follows the C++11 standard, and thus older 
compilers tend to fail the compilation process. Therefore, we strongly recommend
users to satisfy the following requirements:

- **CMake** - version 3.2.3 or higher
  
And, one of the following:
- **g++** - version 5.0 or higher
- **clang** - version 3.6 or higher

## Dependencies

### Required Dependencies

- [SimGrid](https://framagit.org/simgrid/simgrid/uploads/8f3c1608d9420d92c668534633c1208d/SimGrid-3.22.2.tar.gz) - version 3.22.2
- [Lemon C++ library](http://lemon.cs.elte.hu/) - version 1.3.1 or higher 
- [PugiXML](http://pugixml.org/) - version 1.8 or higher 
- [JSON for Modern C++](https://github.com/nlohmann/json) - version 3.1.0 or higher 

### Optional Dependencies
- [Google Test](https://github.com/google/googletest) - version 1.8 or higher (only required for running test cases)
- [Doxygen](http://www.doxygen.org) - version 1.8 or higher (only required for generating documentation)
- [Batsched](https://gitlab.inria.fr/batsim/batsched) - only needed for batch-scheduled resource simulation

## Building From Source

If all dependencies are installed, compiling and installing WRENCH is as simple as running:

```bash
cmake .
make
make install  # try "sudo make install" if you don't have the permission to write
```

For enabling the use of Batsched:
```bash
cmake -DENABLE_BATSCHED=on .
make
make install  # try "sudo make install" if you don't have the permission to write
```

## Get in Touch

The main channel to reach the WRENCH team is via the support email: 
[support@wrench-project.org](mailto:support@wrench-project.org).

**Bug Report / Feature Request:** our preferred channel to report a bug or request a feature is via  
WRENCH's [Github Issues Track](https://github.com/wrench-project/wrench/issues).

## Citing WRENCH

When citing WRENCH, please use the following paper. You should also actually read that paper, as 
it provides a recent and general overview on the framework.

```latex
@inproceedings{wrench,
  title = {WRENCH: A Framework for Simulating Workflow Management Systems},
  author = {Casanova, Henri and Pandey, Suraj and Oeth, James and Tanaka, Ryan and Suter, Frederic and Ferreira da Silva, Rafael},
  booktitle = {13th Workshop on Workflows in Support of Large-Scale Science (WORKS'18)},
  year = {2018},
  pages = {74--85},
  doi = {10.1109/WORKS.2018.00013}
}
```

[travis-badge]:             https://travis-ci.org/wrench-project/wrench.svg?branch=master
[travis-link]:              https://travis-ci.org/wrench-project/wrench
[license-badge]:            https://img.shields.io/badge/License-LGPL%20v3-blue.svg
[coveralls-badge]:          https://coveralls.io/repos/github/wrench-project/wrench/badge.svg?branch=master
[coveralls-link]:           https://coveralls.io/github/wrench-project/wrench?branch=master
[release-badge]:            https://img.shields.io/github/release/wrench-project/wrench/all.svg
[release-link]:             https://github.com/wrench-project/wrench/releases
[codacy-badge]:             https://api.codacy.com/project/badge/Grade/212b95f0b0954fb8b49ab3b90ed0df60
[codacy-link]:              https://www.codacy.com/app/WRENCH/wrench?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=wrench-project/wrench&amp;utm_campaign=Badge_Grade
[sonarcloud-badge]:         https://sonarcloud.io/api/project_badges/measure?project=wrench&metric=ncloc
[sonarcloud-link]:          https://sonarcloud.io/dashboard?id=wrench
[cii-badge]:                https://bestpractices.coreinfrastructure.org/projects/2357/badge
[cii-link]:                 https://bestpractices.coreinfrastructure.org/projects/2357
