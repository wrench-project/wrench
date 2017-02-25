cmake_minimum_required(VERSION 3.6)
project(simulation)

set(CMAKE_CXX_STANDARD 11)

include_directories(/usr/include /usr/local/include /opt/local/include)

link_libraries(simgrid emon)

set(SOURCE_FILES main.cpp
        workflow/Workflow.cpp
        workflow/Workflow.h
        exception/WRENCHException.h
        workflow/WorkflowTask.h
        workflow/WorkflowTask.cpp
        workflow/WorkflowFile.h
        platform/Platform.cpp
        platform/Platform.h
        simulation/Simulation.cpp
        simulation/Simulation.h
        wrench.h
        compute_services/ComputeService.cpp
        compute_services/ComputeService.h
        simgrid_util/DaemonWithMailbox.cpp
        simgrid_util/DaemonWithMailbox.h
        compute_services/sequential_task_executor/SequentialTaskExecutor.cpp
        compute_services/sequential_task_executor/SequentialTaskExecutor.h
        compute_services/sequential_task_executor/SequentialTaskExecutorDaemon.cpp
        compute_services/sequential_task_executor/SequentialTaskExecutorDaemon.h
        simgrid_util/Message.cpp
        simgrid_util/Message.h
        simgrid_util/Mailbox.cpp
        simgrid_util/Mailbox.h
        wms/WMS.cpp
        wms/WMS.h
        wms/sequential_random_WMS/SequentialRandomWMS.cpp
        wms/sequential_random_WMS/SequentialRandomWMS.h
        wms/sequential_random_WMS/SequentialRandomWMSDaemon.h
        wms/sequential_random_WMS/SequentialRandomWMSDaemon.cpp
        simgrid_util/Computation.cpp
        simgrid_util/Computation.h
        simgrid_util/Host.cpp
        simgrid_util/Host.h
        simgrid_util/Environment.cpp
        simgrid_util/Environment.h
        simgrid_util/Clock.cpp
        simgrid_util/Clock.h)

add_executable(simulation ${SOURCE_FILES})