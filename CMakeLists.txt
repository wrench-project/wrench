cmake_minimum_required(VERSION 3.2)
message(STATUS "Cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")

project(wrench CXX)

add_definitions("-Wall -Wno-unused-variable -Wno-unused-private-field")

set(CMAKE_CXX_STANDARD 11)

# build the version number
set(WRENCH_VERSION_MAJOR "1")
set(WRENCH_VERSION_MINOR "0")
set(WRENCH_VERSION_PATCH "0")
set(WRENCH_VERSION_EXTRA "beta")

if (${WRENCH_VERSION_PATCH} EQUAL "0")
    set(WRENCH_RELEASE_VERSION "${WRENCH_VERSION_MAJOR}.${WRENCH_VERSION_MINOR}")
else ()
    set(WRENCH_RELEASE_VERSION "$(WRENCH_VERSION_MAJOR}.${WRENCH_VERSION_MINOR}.${WRENCH_VERSION_PATCH}")
endif ()

if (NOT ${WRENCH_VERSION_EXTRA} EQUAL "")
    set(WRENCH_RELEASE_VERSION "${WRENCH_RELEASE_VERSION}-${WRENCH_VERSION_EXTRA}")
endif ()


include_directories(src/wrench/ include/ /usr/include /usr/local/include /opt/local/include /usr/local/include/nlohmann)
#include_directories(src/wrench/ include/ /usr/include /usr/local/include /Users/casanova/Home/PROJECTS/SIMGRID/SimGrid-3.17 /opt/local/include /usr/local/include/nlohmann)


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
        include/wrench/workflow/Workflow.h
        include/wrench/workflow/WorkflowFile.h
        include/wrench/workflow/WorkflowTask.h
        include/wrench/workflow/job/WorkflowJob.h
        include/wrench/workflow/job/StandardJob.h
        include/wrench/workflow/job/PilotJob.h
        include/wrench/workflow/execution_events/FailureCause.h
        include/wrench/workflow/execution_events/WorkflowExecutionEvent.h
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
        include/wrench/services/compute/multihost_multicore/MultihostMulticoreComputeService.h
        include/wrench/services/compute/multihost_multicore/MultihostMulticoreComputeServiceProperty.h
        include/wrench/services/compute/cloud/CloudService.h
        include/wrench/services/compute/cloud/CloudServiceProperty.h
        include/wrench/services/storage/StorageService.h
        include/wrench/services/storage/StorageServiceProperty.h
        include/wrench/services/storage/simple/SimpleStorageService.h
        include/wrench/services/storage/simple/SimpleStorageServiceProperty.h
        include/wrench/services/file_registry/FileRegistryService.h
        include/wrench/services/file_registry/FileRegistryServiceProperty.h
        include/wrench/services/network_proximity/NetworkProximityService.h
        include/wrench/services/network_proximity/NetworkProximityServiceProperty.h
        include/wrench/services/network_proximity/NetworkDaemons.h
        include/wrench/simulation/Simulation.h
        include/wrench/simulation/SimulationTimestampTypes.h
        include/wrench/simulation/SimulationOutput.h
        include/wrench/simulation/SimulationTimestamp.h
        include/wrench/simulation/SimulationTrace.h
        include/wrench.h
        include/wrench-dev.h
        include/wrench/services/compute/batch/BatchJob.h
        include/wrench/services/compute/batch/BatchServiceMessage.h
        include/wrench/services/compute/batch/BatchService.h
        include/wrench/services/compute/batch/BatchServiceProperty.h
        include/wrench/services/compute/batch/BatchRequestReplyProcess.h
        include/wrench/services/compute/batch/BatchNetworkListener.h
        include/wrench/util/WorkflowUtil.h
        include/wrench/services/helpers/Alarm.h
        include/wrench/util/PointerUtil.h
        include/wrench/util/MessageManager.h
        include/wrench/util/TraceFileLoader.h
        )

# source files
set(SOURCE_FILES
        src/wrench/simulation/SimulationMessage.cpp
        src/wrench/simulation/SimulationMessage.h
        src/wrench/workflow/execution_events/WorkflowExecutionEvent.cpp
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
        src/wrench/services/compute/ComputeService.cpp
        src/wrench/services/compute/multihost_multicore/MultihostMulticoreComputeService.cpp
        src/wrench/workflow/job/PilotJob.cpp
        src/wrench/managers/JobManager.cpp
        src/wrench/simulation/Simulation.cpp
        src/wrench/workflow/job/WorkflowJob.cpp
        src/wrench/workflow/job/StandardJob.cpp
        src/wrench/simulation/SimulationTimestamp.cpp
        src/wrench/simulation/SimulationTrace.cpp
        src/wrench/simulation/SimulationOutput.cpp
        src/wrench/services/file_registry/FileRegistryService.cpp
        src/wrench/services/storage/StorageService.cpp
        src/wrench/services/storage/simple/SimpleStorageService.cpp
        src/wrench/managers/DataMovementManager.cpp
        src/wrench/services/Service.cpp
        src/wrench/services/ServiceProperty.cpp
        src/wrench/services/compute/ComputeServiceProperty.cpp
        src/wrench/services/storage/StorageServiceProperty.cpp
        src/wrench/services/file_registry/FileRegistryServiceProperty.cpp
        src/wrench/services/compute/multihost_multicore/MultihostMulticoreComputeServiceProperty.cpp
        src/wrench/services/storage/simple/SimpleStorageServiceProperty.cpp
        src/wrench/workflow/execution_events/FailureCause.cpp
        src/wrench/services/ServiceMessage.cpp
        src/wrench/services/ServiceMessage.h
        src/wrench/services/compute/ComputeServiceMessage.cpp
        src/wrench/services/compute/ComputeServiceMessage.h
        src/wrench/services/compute/multihost_multicore/MulticoreComputeServiceMessage.cpp
        src/wrench/services/compute/multihost_multicore/MulticoreComputeServiceMessage.h
        src/wrench/services/storage/StorageServiceMessage.cpp
        src/wrench/services/storage/StorageServiceMessage.h
        src/wrench/services/file_registry/FileRegistryMessage.cpp
        src/wrench/services/file_registry/FileRegistryMessage.h
        src/wrench/services/compute/cloud/CloudService.cpp
        src/wrench/services/compute/cloud/CloudServiceProperty.cpp
        src/wrench/services/compute/cloud/CloudServiceMessage.h
        src/wrench/services/compute/cloud/CloudServiceMessage.cpp
        src/wrench/simgrid_S4U_util/S4U_PendingCommunication.cpp
        src/wrench/services/storage/simple/IncomingFile.cpp
        include/wrench/services/storage/simple/IncomingFile.h
        src/wrench/services/compute/standard_job_executor/ComputeThread.h
        src/wrench/services/compute/standard_job_executor/ComputeThread.cpp
        src/wrench/services/compute/standard_job_executor/Workunit.cpp
        src/wrench/services/compute/standard_job_executor/WorkunitMulticoreExecutor.cpp
        src/wrench/services/compute/standard_job_executor/StandardJobExecutorMessage.h
        src/wrench/services/compute/standard_job_executor/StandardJobExecutorMessage.cpp
        src/wrench/services/compute/standard_job_executor/StandardJobExecutor.cpp
        src/wrench/services/compute/standard_job_executor/StandardJobExecutorProperty.cpp
        src/wrench/services/network_proximity/NetworkProximityService.cpp
        src/wrench/services/network_proximity/NetworkProximityServiceProperty.cpp
        src/wrench/services/network_proximity/NetworkProximityMessage.cpp
        src/wrench/services/network_proximity/NetworkProximityMessage.h
        src/wrench/services/network_proximity/NetworkDaemons.cpp
        src/wrench/services/compute/batch/BatchJob.cpp
        src/wrench/services/compute/batch/BatchServiceMessage.cpp
        src/wrench/services/compute/batch/BatchService.cpp
        src/wrench/services/compute/batch/BatchServiceProperty.cpp
        src/wrench/services/compute/batch/BatchRequestReplyProcess.cpp
        src/wrench/services/compute/batch/BatchNetworkListener.cpp
        src/wrench/util/WorkflowUtil.cpp
        src/wrench/services/helpers/Alarm.cpp
        src/wrench/util/PointerUtil.cpp
        src/wrench/util/MessageManager.cpp
        src/wrench/util/TraceFileLoader.cpp
        )

# test files
set(TEST_FILES
        test/main.cpp
        test/constructor_exceptions/MessageConstructorTest.cpp
        test/workflow/WorkflowTest.cpp
        test/workflow/WorkflowFileTest.cpp
        test/workflow/WorkflowTaskTest.cpp
        #        test/wms/optimizations/dynamic/FailureDynamicClusteringTest.cpp
        #        test/wms/optimizations/static/SimplePipelineClusteringTest.cpp
        #        test/wms/scheduler/pilot_job/CriticalPathSchedulerTest.cpp
        test/simulation/OneTaskTest.cpp
        test/simulation/SimpleStorageServiceFunctionalTest.cpp
        test/simulation/SimpleStorageServicePerformanceTest.cpp
        test/simulation/MultihostMulticoreComputeServiceTestStandardJobs.cpp
        test/simulation/MultihostMulticoreComputeServiceTestPilotJobs.cpp
        test/simulation/MultihostMulticoreComputeServiceSchedulingTest.cpp
        test/simulation/TestWithFork.h
        test/simulation/StandardJobExecutorTest.cpp
        test/simulation/NetworkProximityTest.cpp
        test/simulation/BatchServiceTest.cpp
        test/simulation/CloudServiceTest.cpp
        test/simulation/NoopScheduler.cpp
        test/simulation/NoopScheduler.h)

# wrench library
find_library(SIMGRID_LIBRARY NAMES simgrid)
find_library(PUGIXML_LIBRARY NAMES pugixml)
find_library(LEMON_LIBRARY NAMES emon)
find_library(GTEST_LIBRARY NAMES gtest)

add_library(wrench STATIC ${SOURCE_FILES})
set_target_properties(wrench PROPERTIES VERSION ${WRENCH_RELEASE_VERSION})
target_link_libraries(wrench ${SIMGRID_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})

install(TARGETS wrench DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)

# generate unit tests
add_executable(unit_tests EXCLUDE_FROM_ALL ${SOURCE_FILES} ${HEADER_FILES} ${TEST_FILES})
target_link_libraries(unit_tests ${GTEST_LIBRARY} wrench -lpthread -lm -lzmq)
set_target_properties(unit_tests PROPERTIES COMPILE_FLAGS "-g -O0 --coverage")
set_target_properties(unit_tests PROPERTIES LINK_FLAGS "--coverage")
add_custom_command(TARGET unit_tests COMMAND find . -name *.gcda -delete)


include(${CMAKE_HOME_DIRECTORY}/tools/cmake/DefinePackages.cmake)


# build examples
foreach (cmakefile ${CMAKEFILES_TXT})
    string(REPLACE "/CMakeLists.txt" "" repository ${cmakefile})
    add_subdirectory("${CMAKE_HOME_DIRECTORY}/${repository}")
endforeach ()


# build documentation
include(${CMAKE_HOME_DIRECTORY}/tools/cmake/Documentation.cmake)
