Getting started                        {#getting-started}
============

Once you've have installed the wrench library, following the instructions
on the [installation page](@ref install),  you are ready to create a WRENCH
simulator.  **information on what can be simulated and how to do it are
provided in the [WRENCH 101](@ref wrench-101) and [WRENCH 102](@ref wrench-102) 
pages. this page is only about the logistics of setting up a simulator project.**

[TOC]

# using the wrench initialization tool #      {#getting-started-wrench-init}

the `wrench-init` tool is a project generator built with WRENCH, which creates a simple
project structure as follows:

~~~~~~~~~~~~~{.sh}
project-folder/
├── cmakelists.txt
├── readme.md
├── src/
│   ├── simplesimulator.cpp
│   ├── simplestandardjobscheduler.cpp
│   ├── simplestandardjobscheduler.h
│   ├── simplewms.cpp
│   └── simplewms.h 
├── test/
├── doc/
├── build/
└── data/
    ├── platform-files/
    └── workflow-files/
~~~~~~~~~~~~~

the `simplesimulator.cpp` source file contains the class representing the simulator 
(either cloud or batch). `simplestandardjobscheduler.h` and `simplestandardjobscheduler.cpp`
contain a simple implementation for a `wrench::StandardJobScheduler`; `simplewms.h`
and `simplewms.cpp` denote the implementation of a simple workflow management system.
example platform and workflow files are also generated into the `data` folder. these
files provide the minimum necessary implementation for a WRENCH-enabled simulator.

the `wrench-init` tool only requires a single argument, the name of the folder where
the project skeleton will be generated: 

~~~~~~~~~~~~~{.sh}
$ wrench-init <project_folder>
~~~~~~~~~~~~~
 
additional options supported by the tool can be found by using the `wrench-init --help` 
command.

# Creating a CMakeLists.txt file by hand##                {#getting-started-cmakelists}

alternately, you can do a manual setup, i.e., create your own Cmake project. 
below is an example of a `cmakelists.txt` file that can be used as a basic
template:

~~~~~~~~~~~~~{.cmake}
cmake_minimum_required(version 3.2)
message(status "cmake version ${cmake_major_version}.${cmake_minor_version}.${cmake_patch_version}")

project(your_project_name)

add_definitions("-wall -wno-unused-variable -wno-unused-private-field")

set(cmake_cxx_standard 11)

# include directories for dependencies and wrench libraries
include_directories(src/ /usr/local/include /usr/local/include/wrench)

# source files
set(source_files
        src/main.cpp
        )

# test files
set(test_files
        )

# wrench library and dependencies
find_library(wrench_library names wrench)
find_library(simgrid_library names simgrid)
find_library(pugixml_library names pugixml)
find_library(gtest_library names gtest)

# generating the executable
add_executable(my-executable ${source_files})
target_link_libraries(my-executable 
                        ${wrench_library} 
                        ${simgrid_library} 
                        ${pugixml_library} 
                     )

install(targets my-executable destination bin)

# generating unit tests
add_executable(unit_tests exclude_from_all 
                 ${source_files} 
                 ${test_files}
              )
target_link_libraries(unit_tests 
                        ${gtest_library} wrench -lpthread -lm
                     )
~~~~~~~~~~~~~

# Example WRENCH simulators  #         {#getting-started-example}

the examples in the ```examples``` directory provide good starting points
for developing your own simulators.  typing `make` in the top-level
directory compiles the examples in the ```examples``` directory.

let us run the ```examples/basic-examples/bare-metal-bag-of-tasks``` by 
navigating to that directory and typing:

~~~~~~~~~~~~~{.sh}
./wrench-example-bare-metal-bag-of-tasks 6 ./two_hosts.xml --wrench-no-logs --log=custom_wms.threshold=info
~~~~~~~~~~~~~

you should see some output in the terminal. the output in white is
produced by he simulator implemented with the WRENCH user API.  the output
in green is produced by the  workflow management system implemented with
the WRENCH developer API.

although you can inspect the codes of the examples on your own, we highly
recommend that you go through the [WRENCH 101](@ref wrench-101) and 
[WRENCH 102](@ref wrench-102) pages first. these pages make direct references to the
examples, a description of which is available in ```examples/readme.md```
in the WRENCH distribution.

---
