cmake_minimum_required(VERSION 3.6)
project(simulation)

set(CMAKE_CXX_STANDARD 11)

include_directories(/usr/include /usr/local/include /opt/local/include)

link_libraries(simgrid emon)

set(SOURCE_FILES main.cpp
        workflow/Workflow.cpp
        workflow/Workflow.h
        Exception/Exception.h
        workflow/WorkflowTask.h
        workflow/WorkflowTask.cpp
        workflow/WorkflowFile.h
        platform/Platform.cpp
        platform/Platform.h simulation/Simulation.cpp simulation/Simulation.h wrench.h computation/ComputeService.cpp computation/ComputeService.h simgrid_util/SimulatedService.cpp simgrid_util/SimulatedService.h)

add_executable(simulation ${SOURCE_FILES})