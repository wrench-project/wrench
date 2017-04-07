cmake_minimum_required(VERSION 3.2)
project(wrench)

set(CMAKE_CXX_STANDARD 11)

include_directories(src/wrench/ include /usr/include /usr/local/include /opt/local/include)

set(SOURCE_FILES
        src/wrench/exception/WRENCHException.h
        src/wrench/simulation/SimulationMessage.cpp
        src/wrench/simulation/SimulationMessage.h
        src/wrench/workflow_execution_events/WorkflowExecutionEvent.cpp
        src/wrench/workflow_execution_events/WorkflowExecutionEvent.h
        src/wrench/simgrid_S4U_util/S4U_DaemonWithMailbox.cpp
        src/wrench/simgrid_S4U_util/S4U_DaemonWithMailbox.h
        src/wrench/simgrid_S4U_util/S4U_DaemonWithMailboxActor.cpp
        src/wrench/simgrid_S4U_util/S4U_DaemonWithMailboxActor.h
        src/wrench/simgrid_S4U_util/S4U_Simulation.cpp
        src/wrench/simgrid_S4U_util/S4U_Simulation.h
        src/wrench/simgrid_S4U_util/S4U_Mailbox.cpp
        src/wrench/simgrid_S4U_util/S4U_Mailbox.h
        src/wrench/logging/ColorLogging.h
        src/wrench/logging/ColorLogging.cpp
        src/wrench/workflow/Workflow.cpp
        src/wrench/workflow/Workflow.h
        src/wrench/workflow/WorkflowTask.h
        src/wrench/workflow/WorkflowTask.cpp
        src/wrench/workflow/WorkflowFile.h
        src/wrench/workflow/WorkflowFile.cpp
        src/wrench/wms/WMS.h
        src/wrench/wms/engine/EngineDaemon.h
        src/wrench/wms/engine/EngineTmpl.h
        src/wrench/wms/engine/EngineFactory.h
        src/wrench/wms/engine/EngineFactory.cpp
        src/wrench/wms/engine/simple_wms/SimpleWMSDaemon.h
        src/wrench/wms/engine/simple_wms/SimpleWMSDaemon.cpp
        src/wrench/wms/engine/simple_wms/SimpleWMS.h
        src/wrench/wms/engine/simple_wms/SimpleWMS.cpp
        src/wrench/wms/scheduler/Scheduler.h
        src/wrench/wms/scheduler/SchedulerTmpl.h
        src/wrench/wms/scheduler/SchedulerFactory.h
        src/wrench/wms/scheduler/SchedulerFactory.cpp
        src/wrench/wms/scheduler/RandomScheduler.h
        src/wrench/wms/scheduler/RandomScheduler.cpp
        src/wrench/wms/scheduler/MinMinScheduler.h
        src/wrench/wms/scheduler/MinMinScheduler.cpp
        src/wrench/wms/scheduler/MaxMinScheduler.h
        src/wrench/wms/scheduler/MaxMinScheduler.cpp
        src/wrench/compute_services/ComputeService.cpp
        src/wrench/compute_services/ComputeService.h
        src/wrench/helper_daemons/sequential_task_executor/SequentialTaskExecutor.cpp
        src/wrench/helper_daemons/sequential_task_executor/SequentialTaskExecutor.h
        src/wrench/compute_services/multicore_job_executor/MulticoreJobExecutor.cpp
        src/wrench/compute_services/multicore_job_executor/MulticoreJobExecutor.h
        src/wrench/workflow_job/PilotJob.cpp
        src/wrench/workflow_job/PilotJob.h
        src/wrench/job_manager/JobManager.cpp
        src/wrench/job_manager/JobManager.h
        src/wrench/simulation/Simulation.cpp
        src/wrench/simulation/Simulation.h
        src/wrench/main.cpp
        src/wrench/workflow_job/WorkflowJob.cpp
        src/wrench/workflow_job/WorkflowJob.h
        src/wrench/workflow_job/StandardJob.cpp
        src/wrench/workflow_job/StandardJob.h
        )

set(INCLUDE_FILES include/wrench.h)

find_library(SIMGRID_LIB NAMES simgrid)
find_library(PUGIXML_LIB NAMES pugixml)
find_library(LEMON_LIB NAMES emon)

#Generate the static library from the sources
add_library(wrench STATIC ${SOURCE_FILES})
target_link_libraries(wrench ${SIMGRID_LIB} ${LEMON_LIB} ${PUGIXML_LIB})
install(TARGETS wrench DESTINATION lib)
install(FILES ${INCLUDE_FILES} DESTINATION include/wrench)

# TODO: temporary simulation test binary
add_executable(simulation ${SOURCE_FILES})
target_link_libraries(simulation ${SIMGRID_LIB} ${LEMON_LIB} ${PUGIXML_LIB})
install(TARGETS simulation DESTINATION bin)