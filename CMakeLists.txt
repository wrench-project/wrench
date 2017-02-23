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
        simgrid_util/SimgridMessages.cpp
        simgrid_util/SimgridMessages.h
        simgrid_util/SimgridMailbox.cpp
        simgrid_util/SimgridMailbox.h
        wms/WMS.cpp
        wms/WMS.h
        wms/simple_WMS/SimpleWMS.cpp
        wms/simple_WMS/SimpleWMS.h
        wms/simple_WMS/SimpleWMSDaemon.h
        wms/simple_WMS/SimpleWMSDaemon.cpp
        )

add_executable(simulation ${SOURCE_FILES})