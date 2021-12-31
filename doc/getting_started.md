Getting started                        {#getting-started}
============

Once you have installed the WRENCH library, following the instructions
on the [installation page](@ref install),  you are ready to create a WRENCH
simulator.  **Information on what can be simulated and how to do it are
provided in the [WRENCH 101](@ref wrench-101) and [WRENCH 102](@ref wrench-102) 
pages. This page is only about the logistics of setting up a simulator project.**

[TOC]

# Using the WRENCH initialization tool #      {#getting-started-wrench-init}

The `wrench-init` tool is a project generator built with WRENCH, which creates a simple
project structure as follows:

~~~~~~~~~~~~~{.sh}
project-folder/
├── CMakeLists.txt
├── CMakeModules
│   └── FindSimGrid.cmake
├── src/
│   ├── Simulator.cpp
│   ├── Controller.cpp
├── include/
│   └── Controller.h 
├── build/
└── data/
    └── platform.xml
~~~~~~~~~~~~~

The `Simulator.cpp` source file contains the `main()` function of the simulator, which
initializes a simulated platform and services running on this platform;
`Controller.h` and `Controller.cpp` contain the implementation of an execution
controller, which executes a workflow on the available services. The simulator
takes as command-line argument a path to a platform description file in XML, 
which is available in `data/platform.xml`. These
files provide the minimum necessary implementation for a WRENCH-enabled simulator.

The `wrench-init` tool only requires a single argument, the name of the folder where
the project skeleton will be generated: 

~~~~~~~~~~~~~{.sh}
$ wrench-init <project_folder>
~~~~~~~~~~~~~
 
Additional options supported by the tool can be found by using the `wrench-init --help` 
command.

# Creating a CMakeLists.txt file by hand##                {#getting-started-cmakelists}

Alternatively, you can do a manual setup, i.e., create your own Cmake project. 
Below is an example of a `CMakeLists.txt` file that can be used as a basic
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

The examples in the `examples` directory provide good starting points
for developing your own simulators.  Examples are provided for the generic
"action" API as well as for the "workflow" API, and are built along with the
WRENCH library and tools. 

For instance, the `examples/action_api/basic-examples/bare-metal-bag-of-actions` example can be executed 
as: 

~~~~~~~~~~~~~{.sh}
$ wrench-example-bare-metal-bag-of-actions 6 two_hosts.xml --log=custom_wms.threshold=info
~~~~~~~~~~~~~

(File `two_hosts.xml` is in the `examples/action_api/basic-examples/bare-metal-bag-of-actions` directory.)
You should see some output in the terminal. The output in white is
produced by the simulator's main function. The output
in green is produced by the execution controller implemented with
the WRENCH developer API.

Although you can inspect the codes of the examples on your own, we highly
recommend that you go through the [WRENCH 101](@ref wrench-101) and 
[WRENCH 102](@ref wrench-102) pages first. These pages make direct references to the
examples, a description of which is available in `examples/README.md`
in the WRENCH distribution.
