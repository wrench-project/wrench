cmake_minimum_required(VERSION 3.2)
project(wrench)

add_definitions("-Wall -Wno-unused-variable -Wno-unused-private-field")

set(CMAKE_CXX_STANDARD 11)

include_directories(src/wrench/ include/ /usr/include /usr/local/include /opt/local/include)

# library header files
set(HEADER_FILES
        include/wrench/exceptions/WorkflowExecutionException.h
        include/wrench/simgrid_S4U_util/S4U_Simulation.h
        include/wrench/simgrid_S4U_util/S4U_Daemon.h
        include/wrench/simgrid_S4U_util/S4U_PendingCommunication.h
        include/wrench/logging/TerminalOutput.h
        include/wrench/wms/WMS.h
        include/wrench/wms/StaticOptimization.h
        include/wrench/wms/DynamicOptimization.h
        include/wrench/wms/scheduler/PilotJobScheduler.h
        include/wrench/wms/scheduler/Scheduler.h
        include/wrench/wms/scheduler/RandomScheduler.h
        include/wrench/workflow/Workflow.h
        include/wrench/workflow/WorkflowFile.h
        include/wrench/workflow/WorkflowTask.h
        include/wrench/workflow/WorkflowJob.h
        include/wrench/workflow/StandardJob.h
        include/wrench/workflow/FailureCause.h
        include/wrench/workflow/WorkflowExecutionEvent.h
        include/wrench/managers/JobManager.h
        include/wrench/managers/DataMovementManager.h
        include/wrench/services/Service.h
        include/wrench/services/ServiceProperty.h
        include/wrench/services/compute/ComputeService.h
        include/wrench/services/compute/ComputeServiceProperty.h
        include/wrench/services/compute/standard_job_executor/Workunit.h
        include/wrench/services/compute/standard_job_executor/WorkunitMulticoreExecutor.h
        include/wrench/services/compute/standard_job_executor/StandardJobExecutor.h
        include/wrench/services/compute/standard_job_executor/StandardJobExecutorProperty.h
        include/wrench/services/compute/MulticoreComputeService.h
        include/wrench/services/compute/MulticoreComputeServiceProperty.h
        include/wrench/services/storage/StorageService.h
        include/wrench/services/storage/StorageServiceProperty.h
        include/wrench/services/storage/SimpleStorageService.h
        include/wrench/services/storage/SimpleStorageServiceProperty.h
        include/wrench/services/file_registry/FileRegistryService.h
        include/wrench/services/file_registry/FileRegistryServiceProperty.h
        include/wrench/services/network_proximity/NetworkProximityService.h
        include/wrench/services/network_proximity/NetworkQueryServiceProperty.h
        include/wrench/services/network_proximity/NetworkDaemons.h
        include/wrench/simulation/Simulation.h
        include/wrench/simulation/SimulationTimestampTypes.h
        include/wrench/simulation/SimulationOutput.h
        include/wrench/simulation/SimulationTimestamp.h
        include/wrench/simulation/SimulationTrace.h
        include/wrench.h
        include/wrench-dev.h)

# source files
set(SOURCE_FILES
        src/wrench/simulation/SimulationMessage.cpp
        src/wrench/simulation/SimulationMessage.h
        src/wrench/workflow_execution_events/WorkflowExecutionEvent.cpp
        src/wrench/simgrid_S4U_util/S4U_Daemon.cpp
        src/wrench/simgrid_S4U_util/S4U_DaemonActor.cpp
        src/wrench/simgrid_S4U_util/S4U_DaemonActor.h
        src/wrench/simgrid_S4U_util/S4U_Simulation.cpp
        src/wrench/simgrid_S4U_util/S4U_Mailbox.cpp
        src/wrench/simgrid_S4U_util/S4U_Mailbox.h
        src/wrench/logging/TerminalOutput.cpp
        src/wrench/workflow/Workflow.cpp
        src/wrench/workflow/WorkflowTask.cpp
        src/wrench/workflow/WorkflowFile.cpp
        src/wrench/wms/WMS.cpp
        src/wrench/wms/Scheduler.cpp
        src/wrench/wms/scheduler/RandomScheduler.cpp
        src/wrench/services/compute/ComputeService.cpp
        src/wrench/services/compute/multicore_compute_service/MulticoreComputeService.cpp
        src/wrench/workflow_job/PilotJob.cpp
        src/wrench/workflow_job/PilotJob.h
        src/wrench/managers/JobManager.cpp
        src/wrench/simulation/Simulation.cpp
        src/wrench/workflow_job/WorkflowJob.cpp
        src/wrench/workflow_job/StandardJob.cpp
        src/wrench/simulation/SimulationTimestamp.cpp
        src/wrench/simulation/SimulationTrace.cpp
        src/wrench/simulation/SimulationOutput.cpp
        src/wrench/services/file_registry_service/FileRegistryService.cpp
        src/wrench/services/storage_services/StorageService.cpp
        src/wrench/services/storage_services/simple_storage_service/SimpleStorageService.cpp
        src/wrench/managers/DataMovementManager.cpp
        src/wrench/services/Service.cpp
        src/wrench/services/ServiceProperty.cpp
        src/wrench/services/compute/ComputeServiceProperty.cpp
        src/wrench/services/storage_services/StorageServiceProperty.cpp
        src/wrench/services/file_registry_service/FileRegistryServiceProperty.cpp
        src/wrench/services/compute/multicore_compute_service/MulticoreComputeServiceProperty.cpp
        src/wrench/services/storage_services/simple_storage_service/SimpleStorageServiceProperty.cpp
        src/wrench/workflow_execution_events/FailureCause.cpp
        src/wrench/services/ServiceMessage.cpp
        src/wrench/services/ServiceMessage.h
        src/wrench/services/compute/ComputeServiceMessage.cpp
        src/wrench/services/compute/ComputeServiceMessage.h
        src/wrench/services/compute/multicore_compute_service/MulticoreComputeServiceMessage.cpp
        src/wrench/services/compute/multicore_compute_service/MulticoreComputeServiceMessage.h
        src/wrench/services/storage_services/StorageServiceMessage.cpp
        src/wrench/services/storage_services/StorageServiceMessage.h
        src/wrench/services/file_registry_service/FileRegistryMessage.cpp
        src/wrench/services/file_registry_service/FileRegistryMessage.h
        src/wrench/simgrid_S4U_util/S4U_PendingCommunication.cpp
        src/wrench/services/storage_services/simple_storage_service/IncomingFile.cpp
        src/wrench/services/storage_services/simple_storage_service/IncomingFile.h
        src/wrench/services/compute/standard_job_executor/Workunit.cpp
        src/wrench/services/compute/standard_job_executor/WorkunitMulticoreExecutor.cpp
        src/wrench/services/compute/standard_job_executor/StandardJobExecutorMessage.h
        src/wrench/services/compute/standard_job_executor/StandardJobExecutorMessage.cpp
        src/wrench/services/compute/standard_job_executor/StandardJobExecutor.cpp
        src/wrench/services/compute/standard_job_executor/StandardJobExecutorProperty.cpp
        src/wrench/services/network_proximity_services/NetworkProximityService.cpp
        src/wrench/services/network_proximity_services/NetworkQueryServiceProperty.cpp
        src/wrench/services/network_proximity_services/NetworkProximityMessage.cpp
        src/wrench/services/network_proximity_services/NetworkProximityMessage.h
        src/wrench/services/network_proximity_services/NetworkDaemons.cpp
        src/wrench/services/compute/standard_job_executor/ComputeThread.h)

# test files
set(TEST_FILES
        test/main.cpp
        test/workflow/WorkflowTest.cpp
        test/workflow/WorkflowFileTest.cpp
        test/workflow/WorkflowTaskTest.cpp
        #        test/wms/optimizations/dynamic/FailureDynamicClusteringTest.cpp
        #        test/wms/optimizations/static/SimplePipelineClusteringTest.cpp
        #        test/wms/scheduler/pilot_job/CriticalPathSchedulerTest.cpp
        test/simulation/OneTaskTest.cpp
        test/simulation/SimpleStorageServiceFunctionalTest.cpp
        test/simulation/SimpleStorageServicePerformanceTest.cpp
        test/simulation/MulticoreComputeServiceTestStandardJobs.cpp
        test/simulation/MulticoreComputeServiceTestPilotJobs.cpp
        test/simulation/TestWithFork.h
        test/simulation/StandardJobExecutorTest.cpp
        test/simulation/NetworkProximityTest.cpp)

# wrench library
add_library(wrench STATIC ${SOURCE_FILES})
target_link_libraries(wrench -lsimgrid -lpugixml -lemon)

install(TARGETS wrench DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)

# generate unit tests
add_executable(unit_tests EXCLUDE_FROM_ALL ${SOURCE_FILES} ${HEADER_FILES} ${TEST_FILES})
target_link_libraries(unit_tests -lgtest wrench -lpthread -lm)
set_target_properties(unit_tests PROPERTIES COMPILE_FLAGS "-g -O0 --coverage")
set_target_properties(unit_tests PROPERTIES LINK_FLAGS "--coverage")
add_custom_command(TARGET unit_tests COMMAND find . -name *.gcda -delete)

# build examples
include(${CMAKE_HOME_DIRECTORY}/tools/cmake/DefinePackages.cmake)

foreach(cmakefile ${CMAKEFILES_TXT})
    string(REPLACE "/CMakeLists.txt" "" repository ${cmakefile})
    add_subdirectory("${CMAKE_HOME_DIRECTORY}/${repository}")
endforeach()
