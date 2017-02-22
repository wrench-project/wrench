cmake_minimum_required(VERSION 3.6)
project(simulation)

set(CMAKE_CXX_STANDARD 11)

include_directories(/usr/include /usr/local/include /opt/local/include)

link_libraries(simgrid emon)

set(SOURCE_FILES main.cpp
        workflow/Workflow.cpp
        workflow/Workflow.h
        exception/Exception.h
        workflow/WorkflowTask.h
        workflow/WorkflowTask.cpp
        workflow/WorkflowFile.h
        platform/Platform.cpp
        platform/Platform.h simulation/Simulation.cpp simulation/Simulation.h wrench.h computation/ComputeService.cpp computation/ComputeService.h simgrid_util/DaemonWithMailbox.cpp simgrid_util/DaemonWithMailbox.h computation/sequential_task_executor/SequentialTaskExecutor.cpp computation/sequential_task_executor/SequentialTaskExecutor.h computation/sequential_task_executor/SequentialTaskExecutorDaemon.cpp computation/sequential_task_executor/SequentialTaskExecutorDaemon.h)

add_executable(simulation ${SOURCE_FILES})