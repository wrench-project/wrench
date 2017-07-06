cmake_minimum_required(VERSION 3.2)
project(wrench)

add_definitions ("-Wall")

set(CMAKE_CXX_STANDARD 11)

include_directories(src/wrench/ include /usr/include /usr/local/include /opt/local/include)

# source files
set(SOURCE_FILES
        src/wrench/exceptions/WorkflowExecutionException.h
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
        src/wrench/logging/TerminalOutput.h
        src/wrench/logging/TerminalOutput.cpp
        src/wrench/workflow/Workflow.cpp
        src/wrench/workflow/Workflow.h
        src/wrench/workflow/WorkflowTask.h
        src/wrench/workflow/WorkflowTask.cpp
        src/wrench/workflow/WorkflowFile.h
        src/wrench/workflow/WorkflowFile.cpp
        src/wrench/wms/WMS.h
        src/wrench/wms/WMS.cpp
        src/wrench/wms/optimizations/static/StaticOptimization.h
        src/wrench/wms/optimizations/static/SimplePipelineClustering.h
        src/wrench/wms/optimizations/static/SimplePipelineClustering.cpp
        src/wrench/wms/optimizations/dynamic/DynamicOptimization.h
        src/wrench/wms/optimizations/dynamic/FailureDynamicClustering.h
        src/wrench/wms/optimizations/dynamic/FailureDynamicClustering.cpp
        src/wrench/wms/engine/SimpleWMS.h
        src/wrench/wms/engine/SimpleWMS.cpp
        src/wrench/wms/scheduler/Scheduler.h
        src/wrench/wms/scheduler/Scheduler.cpp
        src/wrench/wms/scheduler/RandomScheduler.h
        src/wrench/wms/scheduler/RandomScheduler.cpp
        src/wrench/wms/scheduler/MinMinScheduler.h
        src/wrench/wms/scheduler/MinMinScheduler.cpp
        src/wrench/wms/scheduler/MaxMinScheduler.h
        src/wrench/wms/scheduler/MaxMinScheduler.cpp
        src/wrench/services/compute_services/ComputeService.cpp
        src/wrench/services/compute_services/ComputeService.h
        src/wrench/services/compute_services/multicore_compute_service/MulticoreComputeService.cpp
        src/wrench/services/compute_services/multicore_compute_service/MulticoreComputeService.h
        src/wrench/workflow_job/PilotJob.cpp
        src/wrench/workflow_job/PilotJob.h
        src/wrench/managers/job_manager/JobManager.cpp
        src/wrench/managers/job_manager/JobManager.h
        src/wrench/simulation/Simulation.cpp
        src/wrench/simulation/Simulation.h
        src/wrench/workflow_job/WorkflowJob.cpp
        src/wrench/workflow_job/WorkflowJob.h
        src/wrench/workflow_job/StandardJob.cpp
        src/wrench/workflow_job/StandardJob.h
        src/wrench/simulation/SimulationTimestamp.cpp
        src/wrench/simulation/SimulationTimestamp.h
        src/wrench/simulation/SimulationTrace.cpp
        src/wrench/simulation/SimulationTrace.h
        src/wrench/simulation/SimulationOutput.cpp
        src/wrench/simulation/SimulationOutput.h
        src/wrench/simulation/SimulationTimestampTypes.h
        src/wrench/services/file_registry_service/FileRegistryService.cpp
        src/wrench/services/file_registry_service/FileRegistryService.h
        src/wrench/services/storage_services/StorageService.cpp
        src/wrench/services/storage_services/StorageService.h
        src/wrench/services/storage_services/simple_storage_service/SimpleStorageService.cpp
        src/wrench/services/storage_services/simple_storage_service/SimpleStorageService.h
        src/wrench/managers/data_movement_manager/DataMovementManager.cpp
        src/wrench/managers/data_movement_manager/DataMovementManager.h
        src/wrench/services/Service.cpp
        src/wrench/services/Service.h
        src/wrench/services/ServiceProperty.cpp
        src/wrench/services/compute_services/ComputeServiceProperty.cpp
        src/wrench/services/storage_services/StorageServiceProperty.cpp
        src/wrench/services/file_registry_service/FileRegistryServiceProperty.cpp
        src/wrench/services/compute_services/multicore_compute_service/MulticoreComputeServiceProperty.cpp
        src/wrench/services/storage_services/simple_storage_service/SimpleStorageServiceProperty.cpp
        src/wrench/workflow_execution_events/WorkflowExecutionFailureCause.h
        src/wrench/workflow_execution_events/WorkflowExecutionFailureCause.cpp
        src/wrench/services/ServiceMessage.cpp
        src/wrench/services/ServiceMessage.h
        src/wrench/services/compute_services/ComputeServiceMessage.cpp
        src/wrench/services/compute_services/ComputeServiceMessage.h
        src/wrench/services/compute_services/multicore_compute_service/MulticoreComputeServiceMessage.cpp
        src/wrench/services/compute_services/multicore_compute_service/MulticoreComputeServiceMessage.h
        src/wrench/services/storage_services/StorageServiceMessage.cpp
        src/wrench/services/storage_services/StorageServiceMessage.h
        src/wrench/services/file_registry_service/FileRegistryMessage.cpp
        src/wrench/services/file_registry_service/FileRegistryMessage.h
        src/wrench/services/compute_services/multicore_compute_service/WorkUnitExecutor.cpp
        src/wrench/services/compute_services/multicore_compute_service/WorkUnitExecutor.h
        src/wrench/services/compute_services/multicore_compute_service/WorkUnit.cpp
        src/wrench/services/compute_services/multicore_compute_service/WorkUnit.h
        src/wrench/wms/scheduler/pilot_job/PilotJobScheduler.h
        src/wrench/wms/scheduler/pilot_job/CriticalPathScheduler.h
        src/wrench/wms/scheduler/pilot_job/CriticalPathScheduler.cpp
        src/wrench/simgrid_S4U_util/S4U_PendingCommunication.cpp
        src/wrench/simgrid_S4U_util/S4U_PendingCommunication.h
        src/wrench/services/storage_services/simple_storage_service/IncomingFile.cpp
        src/wrench/services/storage_services/simple_storage_service/IncomingFile.h)

# test files
set(TEST_FILES
        test/main.cpp
        test/workflow/WorkflowTest.cpp
        test/workflow/WorkflowFileTest.cpp
        test/workflow/WorkflowTaskTest.cpp
        test/wms/optimizations/dynamic/FailureDynamicClusteringTest.cpp
        test/wms/optimizations/static/SimplePipelineClusteringTest.cpp
        test/simulation/OneTaskTest.cpp
        test/simulation/SimpleStorageServiceTest.cpp
        test/simulation/MulticoreComputeServiceTestStandardJobs.cpp
        test/simulation/MulticoreComputeServiceTestPilotJobs.cpp
        test/simulation/TestWithFork.h)

set(INCLUDE_FILES include/wrench.h)
set(INCLUDE_DEV_FILES include/wrench-dev.h)

find_library(SIMGRID_LIB NAMES simgrid)
find_library(PUGIXML_LIB NAMES pugixml)
find_library(LEMON_LIB NAMES emon)
find_library(GOOGLE_TEST NAMES gtest)

# generate the static library from the sources
add_library(wrench STATIC ${SOURCE_FILES})
target_link_libraries(wrench ${SIMGRID_LIB} ${LEMON_LIB} ${PUGIXML_LIB})
install(TARGETS wrench DESTINATION lib)
install(FILES ${INCLUDE_FILES} DESTINATION include/wrench)
install(FILES ${INCLUDE_DEV_FILES} DESTINATION include/wrench)

# generate unit tests
add_executable(unit_tests EXCLUDE_FROM_ALL ${SOURCE_FILES} ${TEST_FILES})
target_link_libraries(unit_tests ${GOOGLE_TEST} wrench -lpthread -lm)
set_target_properties(unit_tests PROPERTIES COMPILE_FLAGS "-g -O0 --coverage")
set_target_properties(unit_tests PROPERTIES LINK_FLAGS "--coverage")
add_custom_command(TARGET unit_tests COMMAND find . -name *.gcda -delete)

# TODO: temporary simulation test binary
add_executable(simulation src/wrench/main.cpp)
target_link_libraries(simulation wrench)
install(TARGETS simulation DESTINATION bin)