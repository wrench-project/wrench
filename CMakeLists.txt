cmake_minimum_required(VERSION 3.6)
project(wrench)

set(CMAKE_CXX_STANDARD 11)

include_directories(/usr/include /usr/local/include /opt/local/include)

link_libraries(simgrid emon)

set(SOURCE_FILES main.cpp
        workflow/Workflow.cpp
        workflow/Workflow.h
        workflow/Exception.h
        workflow/WorkflowTask.h
        workflow/WorkflowTask.cpp
        workflow/WorkflowFile.h
        platform/Platform.cpp
        platform/Platform.h)

add_executable(wrench ${SOURCE_FILES})