Getting Started                        {#getting-started}
============

@WRENCHUserDoc <div class="doc-type">User Documentation</div><div class="doc-link">Other: <a href="../developer/getting-started.html">Developer</a> - <a href="../internal/getting-started.html">Internal</a></div> @endWRENCHDoc
@WRENCHDeveloperDoc  <div class="doc-type">Developer Documentation</div><div class="doc-link">Other: <a href="../user/getting-started.html">User</a> - <a href="../internal/getting-started.html">Internal</a></div> @endWRENCHDoc
@WRENCHInternalDoc  <div class="doc-type">Internal Documentation</div><div class="doc-link">Other: <a href="../user/getting-started.html">User</a> -  <a href="../developer/getting-started.html">Developer</a></div> @endWRENCHDoc

[TOC]

The first step is to install the WRENCH library, following the instructions
 on the [installation page](@ref install).

# Running a First Example #         {#getting-started-example}

 Typing `make` in the top-level directory
will compile the examples, and `make install` will put the examples binaries in the `/usr/local/bin` 
folder (for MacOS and most Linux distributions). 


WRENCH provides a simple example in the `examples/simple-example` directory, which 
generates two executables: a cloud-based 
example `wrench-simple-example-cloud`, and a 
batch-system-based (e.g., SLURM) example `wrench-simple-example-batch`.
To run the examples, simply use 
one of the following commands:

~~~~~~~~~~~~~{.sh}
# Runs the cloud-based implementation
wrench-simple-example-cloud \
    <PATH-TO-WRENCH-FOLDER>/examples/simple-example/platform_files/cloud_hosts.xml \
    <PATH-TO-WRENCH-FOLDER>/examples/simple-example/workflow_files/genome.dax

# Runs the batch-based implementation
wrench-simple-example-batch \
    <PATH-TO-WRENCH-FOLDER>/examples/simple-example/platform_files/batch_hosts.xml \
    <PATH-TO-WRENCH-FOLDER>/examples/simple-example/workflow_files/genome.dax
~~~~~~~~~~~~~


## Understanding the Simple Example      {#getting-started-example-simple}

Both versions of the example (cloud of batch) require two command-line arguments: (1) a [SimGrid virtual platform 
description file](https://simgrid.org/doc/latest/platform.html); and
(2) a WRENCH workflow file.

  - **SimGrid simulated platform description file:** 
A [SimGrid](https://simgrid.org) simulation must be provided with the description 
of the platform on which an application execution is to be simulated. This is done via
a platform description file, in XML, that includes definitions of compute hosts, clusters of hosts, 
storage resources, network links, routes between hosts, etc.
A detailed description on how to create a platform description file can be found
[here](https://simgrid.org/doc/latest/platform.html).

  - **WRENCH workflow file:**
WRENCH provides native parsers for [DAX](http://workflowarchive.org) (DAG in XML) 
and [JSON](https://github.com/wrench-project/wrench/tree/master/doc/schemas) workflow description file formats. Refer to 
their respective Web sites for detailed documentation.

The source file for the cloud-based simulator is at `examples/simple-example/SimulatorCloud.cpp`
 and at `examples/simple-example/SimulatorBatch.cpp` for the batch-based example. These source files, which
 are heavily commented, and perform the following:

- The first step is to read and parse the workflow and the platform files, and to
  create a simulation object (`wrench::Simulation`).
- A storage service (`wrench::SimpleStorageService`) is created and deployed on a host.
- A cloud (`wrench::CloudComputeService`) or a batch (`wrench::BatchComputeService`) service is created and
  deployed on a host. Both services are seen by the simulation as compute services
  (`wrench::ComputeService`) – jobs can then be submitted to these services. 
- A Workflow Management System (`wrench::WMS`) is instantiated (in this case the `SimpleWMS`) with a reference to 
  a workflow object (`wrench::Workflow`) and a scheduler (`wrench::Scheduler`). The scheduler implements the
  decision-making algorithms inside the WMS. These algorithms are modularized (so that the same WMS implementation can be iniated
  with various decision-making algorithms in different simulations). The source codes for the schedulers,
  which is of interest to "Developers" (i.e., those users who use the WRENCH Developer API), is in 
  directory `examples/scheduler`. 
- A file registry (`wrench::FileRegistryService`), a.k.a. a file replica catalog, which keeps track of files stored in different storage services, is deployed on a host. 
- Workflow input files are staged on the storage service
- The simulation is launched, executes, and completes.
- Timestamps can be retrieved to analyze the simulated execution.


This simple example can be used as a blueprint for starting a large WRENCH-based
simulation project. The next section provides further details about this process.


@WRENCHNotInternalDoc
# WRENCH Initialization Tool #      {#getting-started-wrench-init}

The `wrench-init` tool is a project generator built with WRENCH, which creates a simple
project structure with example class files, as follows:

~~~~~~~~~~~~~{.sh}
project-folder/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── SimpleSimulator.cpp
│   ├── SimpleStandardJobScheduler.cpp
│   ├── SimpleStandardJobScheduler.h
│   ├── SimpleWMS.cpp
│   └── SimpleWMS.h 
├── test/
├── doc/
├── build/
└── data/
    ├── platform-files/
    └── workflow-files/
~~~~~~~~~~~~~

The `SimpleSimulator.cpp` source file contains the class representing the simulator 
(either cloud or batch). `SimpleStandardJobScheduler.h` and `SimpleStandardJobScheduler.cpp`
contain a simple implementation for a `wrench::StandardJobScheduler`; `SimpleWMS.h`
and `SimpleWMS.cpp` denote the implementation of a simple workflow management system.
Example platform and workflow files are also generated into the `data` folder. These
files provide the minimum necessary implementation for a WRENCH-enabled simulator.

The `wrench-init` tool only requires a single argument, the name of the folder where
the project skeleton will be generated: 

~~~~~~~~~~~~~{.sh}
$ wrench-init <PROJECT_FOLDER>
~~~~~~~~~~~~~
 
Additional options supported by the tool can be found by using the `wrench-init --help` 
command.
@endWRENCHDoc


# Preparing the Environment #         {#getting-started-prep}

@WRENCHNotInternalDoc
## Importing WRENCH ##                {#getting-started-prep-import}

For ease of use, all WRENCH abstractions are accessed via a single 
include statement:

@WRENCHUserDoc
~~~~~~~~~~~~~{.cpp}
#include <wrench.h>
~~~~~~~~~~~~~
@endWRENCHDoc

@WRENCHDeveloperDoc 
~~~~~~~~~~~~~{.cpp}
#include <wrench-dev.h>
~~~~~~~~~~~~~

Note that `wrench-dev.h` is the only necessary include statement to use WRENCH. 
It includes all interfaces and services provided in `wrench.h` 
([user API](../user/getting-started.html)), as well as additional interfaces to develop 
your own algorithms and services.
 
@endWRENCHDoc

## Creating Your CMakeLists.txt File ##                {#getting-started-prep-cmakelists}

Below is an example of a `CMakeLists.txt` file that can be used as a starting 
template for developing a WRENCH application compiled using cmake:

~~~~~~~~~~~~~{.cmake}
cmake_minimum_required(VERSION 3.2)
message(STATUS "Cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")

project(YOUR_PROJECT_NAME)

add_definitions("-Wall -Wno-unused-variable -Wno-unused-private-field")

set(CMAKE_CXX_STANDARD 11)

# include directories for dependencies and WRENCH libraries
include_directories(src/ /usr/local/include /usr/local/include/wrench)

# source files
set(SOURCE_FILES
        src/main.cpp
        )

# test files
set(TEST_FILES
        )

# wrench library and dependencies
find_library(WRENCH_LIBRARY NAMES wrench)
find_library(SIMGRID_LIBRARY NAMES simgrid)
find_library(PUGIXML_LIBRARY NAMES pugixml)
find_library(LEMON_LIBRARY NAMES emon)
find_library(GTEST_LIBRARY NAMES gtest)

# generating the executable
add_executable(my-executable ${SOURCE_FILES})
target_link_libraries(my-executable 
                        ${WRENCH_LIBRARY} 
                        ${SIMGRID_LIBRARY} 
                        ${PUGIXML_LIBRARY} 
                        ${LEMON_LIBRARY}
                     )

install(TARGETS my-executable DESTINATION bin)

# generating unit tests
add_executable(unit_tests EXCLUDE_FROM_ALL 
                 ${SOURCE_FILES} 
                 ${TEST_FILES}
              )
target_link_libraries(unit_tests 
                        ${GTEST_LIBRARY} wrench -lpthread -lm
                     )
~~~~~~~~~~~~~

@endWRENCHDoc

@WRENCHInternalDoc

Internal developers are expected to **contribute** code to WRENCH's core components.
Please, refer to the [API Reference](./annotated.html) to find the detailed 
documentation for WRENCH functions.

> **Note:** It is strongly recommended that WRENCH internal developers (contributors) 
> _fork_ WRENCH's code from the [GitHub repository](http://github.com/wrench-project/wrench),
> and create pull requests with their proposed modifications.


# WRENCH Directory and File Structure #         {#getting-started-structure}

WRENCH follows a standard C++ project directory and files structure:

~~~~~~~~~~~~~{.sh}
.
+-- doc                        # Documentation source files
+-- docs                       # Generated documentation files
+-- examples                   # Examples folder (includes workflows, platform files, and implementations) 
+-- include                    # WRENCH header files - .h files 
+-- src                        # WRENCH source files - .cpp files
+-- test                       # WRENCH test files
+-- tools                      # Tools for supporting documentation generation and release builds
+-- .travis.yml                # Configuration file for Travis Continuous Integration
+-- sonar-project.properties   # Configuration file for Sonar Cloud Continuous Code Quality
+-- LICENSE.md                 # WRENCH license disclaimer
+-- README.md
~~~~~~~~~~~~~

@endWRENCHDoc
