cmake_minimum_required(VERSION 3.2)
message(STATUS "Cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")

project(wrench CXX)

add_definitions("-Wall -Wno-unused-variable -Wno-unused-private-field")
if (ENABLE_BATSCHED)
    add_definitions(-DENABLE_BATSCHED)
endif ()
if (ENABLE_MESSAGE_MANAGER)
    add_definitions(-DMESSAGE_MANAGER)
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/conf/cmake/")
find_package(SimGrid REQUIRED)

set(CMAKE_CXX_STANDARD 11)

# build the version number
set(WRENCH_VERSION_MAJOR "1")
set(WRENCH_VERSION_MINOR "6")
set(WRENCH_VERSION_PATCH "0")
set(WRENCH_VERSION_EXTRA "dev")

if (${WRENCH_VERSION_PATCH} EQUAL "0")
    set(WRENCH_RELEASE_VERSION "${WRENCH_VERSION_MAJOR}.${WRENCH_VERSION_MINOR}")
else ()
    set(WRENCH_RELEASE_VERSION "${WRENCH_VERSION_MAJOR}.${WRENCH_VERSION_MINOR}.${WRENCH_VERSION_PATCH}")
endif ()

if (NOT ${WRENCH_VERSION_EXTRA} STREQUAL "")
    set(WRENCH_RELEASE_VERSION "${WRENCH_RELEASE_VERSION}-${WRENCH_VERSION_EXTRA}")
endif ()

message(STATUS "Building WRENCH Version: ${WRENCH_RELEASE_VERSION}")

include_directories(src/wrench/ include/ ${SimGrid_INCLUDE_DIR}/include /usr/include /usr/local/include)


# For MacOS's MacPorts
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} /opt/local/lib)
include_directories(/opt/local/include)

# library header files
set(HEADER_FILES
        include/wrench-dev.h
        include/wrench.h
        include/wrench/exceptions/WorkflowExecutionException.h
        include/wrench/logging/TerminalOutput.h
        include/wrench/managers/DataMovementManager.h
        include/wrench/managers/JobManager.h
        include/wrench/services/Service.h
        include/wrench/services/ServiceMessage.h
        include/wrench/services/ServiceMessagePayload.h
        include/wrench/services/ServiceProperty.h
        include/wrench/services/compute/ComputeService.h
        include/wrench/services/compute/ComputeServiceMessage.h
        include/wrench/services/compute/ComputeServiceMessagePayload.h
        include/wrench/services/compute/ComputeServiceProperty.h
        include/wrench/services/compute/bare_metal/BareMetalComputeService.h
        include/wrench/services/compute/bare_metal/BareMetalComputeServiceMessagePayload.h
        include/wrench/services/compute/bare_metal/BareMetalComputeServiceProperty.h
        include/wrench/services/compute/batch/BatchComputeService.h
        include/wrench/services/compute/batch/BatchComputeServiceMessage.h
        include/wrench/services/compute/batch/BatchComputeServiceMessagePayload.h
        include/wrench/services/compute/batch/BatchComputeServiceProperty.h
        include/wrench/services/compute/batch/BatchJob.h
        include/wrench/services/compute/batch/BatschedNetworkListener.h
        include/wrench/services/compute/cloud/CloudComputeService.h
        include/wrench/services/compute/cloud/CloudComputeServiceMessagePayload.h
        include/wrench/services/compute/cloud/CloudComputeServiceProperty.h
        include/wrench/services/compute/htcondor/HTCondorCentralManagerService.h
        include/wrench/services/compute/htcondor/HTCondorCentralManagerServiceMessage.h
        include/wrench/services/compute/htcondor/HTCondorCentralManagerServiceMessagePayload.h
        include/wrench/services/compute/htcondor/HTCondorComputeService.h
        include/wrench/services/compute/htcondor/HTCondorComputeServiceMessagePayload.h
        include/wrench/services/compute/htcondor/HTCondorComputeServiceProperty.h
        include/wrench/services/compute/htcondor/HTCondorNegotiatorService.h
        include/wrench/services/compute/standard_job_executor/StandardJobExecutor.h
        include/wrench/services/compute/standard_job_executor/StandardJobExecutorMessagePayload.h
        include/wrench/services/compute/standard_job_executor/StandardJobExecutorProperty.h
        include/wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeService.h
        include/wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeServiceMessagePayload.h
        include/wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeServiceProperty.h
        include/wrench/services/compute/workunit_executor/Workunit.h
        include/wrench/services/compute/workunit_executor/WorkunitExecutor.h
        include/wrench/services/file_registry/FileRegistryService.h
        include/wrench/services/file_registry/FileRegistryServiceMessagePayload.h
        include/wrench/services/file_registry/FileRegistryServiceProperty.h
        include/wrench/services/helpers/Alarm.h
        include/wrench/services/helpers/HostStateChangeDetector.h
        include/wrench/services/helpers/HostStateChangeDetectorMessage.h
        include/wrench/services/helpers/HostStateChangeDetectorProperty.h
        include/wrench/services/helpers/ServiceTerminationDetector.h
        include/wrench/services/helpers/ServiceTerminationDetectorMessage.h
        include/wrench/services/network_proximity/NetworkProximityDaemon.h
        include/wrench/services/network_proximity/NetworkProximityService.h
        include/wrench/services/network_proximity/NetworkProximityServiceMessagePayload.h
        include/wrench/services/network_proximity/NetworkProximityServiceProperty.h
        include/wrench/services/storage/StorageService.h
        include/wrench/services/storage/StorageServiceMessagePayload.h
        include/wrench/services/storage/StorageServiceProperty.h
        include/wrench/services/storage/simple/SimpleStorageService.h
        include/wrench/services/storage/simple/SimpleStorageServiceMessagePayload.h
        include/wrench/services/storage/simple/SimpleStorageServiceProperty.h
        include/wrench/services/storage/storage_helpers/FileLocation.h
        include/wrench/simgrid_S4U_util/S4U_Daemon.h
        include/wrench/simgrid_S4U_util/S4U_Mailbox.h
        include/wrench/simgrid_S4U_util/S4U_PendingCommunication.h
        include/wrench/simgrid_S4U_util/S4U_Simulation.h
        include/wrench/simgrid_S4U_util/S4U_VirtualMachine.h
        include/wrench/simulation/Simulation.h
        include/wrench/simulation/SimulationMessage.h
        include/wrench/simulation/SimulationOutput.h
        include/wrench/simulation/SimulationTimestamp.h
        include/wrench/simulation/SimulationTimestampTypes.h
        include/wrench/simulation/SimulationTrace.h
        include/wrench/simulation/Version.h
        include/wrench/util/MessageManager.h
        include/wrench/util/PointerUtil.h
        include/wrench/util/TraceFileLoader.h
        include/wrench/util/UnitParser.h
        include/wrench/wms/DynamicOptimization.h
        include/wrench/wms/StaticOptimization.h
        include/wrench/wms/WMS.h
        include/wrench/wms/scheduler/PilotJobScheduler.h
        include/wrench/wms/scheduler/PilotJobScheduler.h
        include/wrench/wms/scheduler/StandardJobScheduler.h
        include/wrench/wms/scheduler/StandardJobScheduler.h
        include/wrench/workflow/Workflow.h
        include/wrench/workflow/WorkflowFile.h
        include/wrench/workflow/WorkflowTask.h
        include/wrench/workflow/execution_events/WorkflowExecutionEvent.h
        include/wrench/workflow/execution_events/StandardJobCompletedEvent.h
        include/wrench/workflow/execution_events/StandardJobFailedEvent.h
        include/wrench/workflow/execution_events/PilotJobStartedEvent.h
        include/wrench/workflow/execution_events/PilotJobExpiredEvent.h
        include/wrench/workflow/execution_events/FileCopyCompletedEvent.h
        include/wrench/workflow/execution_events/FileCopyFailedEvent.h
        include/wrench/workflow/execution_events/TimerEvent.h
        include/wrench/workflow/failure_causes/ComputeThreadHasDied.h
        include/wrench/workflow/failure_causes/FailureCause.h
        include/wrench/workflow/failure_causes/FatalFailure.h
        include/wrench/workflow/failure_causes/FileAlreadyBeingCopied.h
        include/wrench/workflow/failure_causes/FileNotFound.h
        include/wrench/workflow/failure_causes/FunctionalityNotAvailable.h
        include/wrench/workflow/failure_causes/HostError.h
        include/wrench/workflow/failure_causes/InvalidDirectoryPath.h
        include/wrench/workflow/failure_causes/JobKilled.h
        include/wrench/workflow/failure_causes/JobTimeout.h
        include/wrench/workflow/failure_causes/JobTypeNotSupported.h
        include/wrench/workflow/failure_causes/NetworkError.h
        include/wrench/workflow/failure_causes/NoScratchSpace.h
        include/wrench/workflow/failure_causes/NotAllowed.h
        include/wrench/workflow/failure_causes/NotEnoughResources.h
        include/wrench/workflow/failure_causes/ServiceIsDown.h
        include/wrench/workflow/failure_causes/ServiceIsSuspended.h
        include/wrench/workflow/failure_causes/StorageServiceNotEnoughSpace.h
        include/wrench/workflow/job/PilotJob.h
        include/wrench/workflow/job/StandardJob.h
        include/wrench/workflow/job/WorkflowJob.h
        )

# source files
set(SOURCE_FILES
        include/wrench/services/compute/batch/batch_schedulers/BatchScheduler.h
        include/wrench/services/compute/batch/batch_schedulers/homegrown/HomegrownBatchScheduler.h
        include/wrench/services/storage/storage_helpers/FileTransferThread.h
        include/wrench/services/storage/storage_helpers/LogicalFileSystem.h
        src/wrench/helper_services/alarm/Alarm.cpp
        src/wrench/helper_services/host_state_change_detector/HostStateChangeDetector.cpp
        src/wrench/helper_services/host_state_change_detector/HostStateChangeDetectorMessage.cpp
        src/wrench/helper_services/host_state_change_detector/HostStateChangeDetectorProperty.cpp
        src/wrench/helper_services/service_termination_detector/ServiceTerminationDetector.cpp
        src/wrench/helper_services/service_termination_detector/ServiceTerminationDetectorMessage.cpp
        src/wrench/helper_services/standard_job_executor/StandardJobExecutor.cpp
        src/wrench/helper_services/standard_job_executor/StandardJobExecutorMessage.cpp
        src/wrench/helper_services/standard_job_executor/StandardJobExecutorMessage.h
        src/wrench/helper_services/standard_job_executor/StandardJobExecutorMessagePayload.cpp
        src/wrench/helper_services/standard_job_executor/StandardJobExecutorProperty.cpp
        src/wrench/helper_services/work_unit_executor/ComputeThread.cpp
        src/wrench/helper_services/work_unit_executor/ComputeThread.h
        src/wrench/helper_services/work_unit_executor/Workunit.cpp
        src/wrench/helper_services/work_unit_executor/WorkunitExecutor.cpp
        src/wrench/logging/TerminalOutput.cpp
        src/wrench/managers/DataMovementManager.cpp
        src/wrench/managers/EnergyMeter.cpp
        src/wrench/managers/JobManager.cpp
        src/wrench/managers/JobManagerMessage.cpp
        src/wrench/managers/JobManagerMessage.h
        src/wrench/services/Service.cpp
        src/wrench/services/ServiceMessage.cpp
        src/wrench/services/ServiceMessagePayload.cpp
        src/wrench/services/ServiceProperty.cpp
        src/wrench/services/compute/ComputeService.cpp
        src/wrench/services/compute/ComputeServiceMessage.cpp
        src/wrench/services/compute/ComputeServiceMessagePayload.cpp
        src/wrench/services/compute/ComputeServiceProperty.cpp
        src/wrench/services/compute/bare_metal/BareMetalComputeService.cpp
        src/wrench/services/compute/bare_metal/BareMetalComputeServiceMessagePayload.cpp
        src/wrench/services/compute/bare_metal/BareMetalComputeServiceProperty.cpp
        src/wrench/services/compute/batch/BatchComputeService.cpp
        src/wrench/services/compute/batch/BatchComputeServiceMessage.cpp
        src/wrench/services/compute/batch/BatchComputeServiceMessagePayload.cpp
        src/wrench/services/compute/batch/BatchComputeServiceProperty.cpp
        src/wrench/services/compute/batch/BatchJob.cpp
        src/wrench/services/compute/batch/BatschedNetworkListener.cpp
        src/wrench/services/compute/batch/batch_schedulers/BatchScheduler.cpp
        src/wrench/services/compute/batch/batch_schedulers/batsched/BatschedBatchScheduler.cpp
        src/wrench/services/compute/batch/batch_schedulers/batsched/BatschedBatchScheduler.h
        src/wrench/services/compute/batch/batch_schedulers/homegrown/HomegrownBatchScheduler.cpp
        src/wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf/BatchJobSet.h
        src/wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf/CONSERVATIVEBFBatchScheduler.cpp
        src/wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf/CONSERVATIVEBFBatchScheduler.h
        src/wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf/NodeAvailabilityTimeLine.cpp
        src/wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf/NodeAvailabilityTimeLine.h
        src/wrench/services/compute/batch/batch_schedulers/homegrown/fcfs/FCFSBatchScheduler.cpp
        src/wrench/services/compute/batch/batch_schedulers/homegrown/fcfs/FCFSBatchScheduler.h
        src/wrench/services/compute/batch/workload_helper_classes/TraceFileLoader.cpp
        src/wrench/services/compute/batch/workload_helper_classes/WorkloadTraceFileReplayer.cpp
        src/wrench/services/compute/batch/workload_helper_classes/WorkloadTraceFileReplayerEventReceiver.cpp
        src/wrench/services/compute/cloud/CloudComputeService.cpp
        src/wrench/services/compute/cloud/CloudComputeServiceMessage.cpp
        src/wrench/services/compute/cloud/CloudComputeServiceMessage.h
        src/wrench/services/compute/cloud/CloudComputeServiceMessagePayload.cpp
        src/wrench/services/compute/cloud/CloudComputeServiceProperty.cpp
        src/wrench/services/compute/htcondor/HTCondorCentralManagerService.cpp
        src/wrench/services/compute/htcondor/HTCondorCentralManagerServiceMessage.cpp
        src/wrench/services/compute/htcondor/HTCondorCentralManagerServiceMessagePayload.cpp
        src/wrench/services/compute/htcondor/HTCondorComputeService.cpp
        src/wrench/services/compute/htcondor/HTCondorNegotiatorService.cpp
        src/wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeService.cpp
        src/wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeServiceMessage.cpp
        src/wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeServiceMessage.h
        src/wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeServiceMessagePayload.cpp
        src/wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeServiceProperty.cpp
        src/wrench/services/file_registry/FileRegistryMessage.cpp
        src/wrench/services/file_registry/FileRegistryMessage.h
        src/wrench/services/file_registry/FileRegistryService.cpp
        src/wrench/services/file_registry/FileRegistryServiceMessagePayload.cpp
        src/wrench/services/file_registry/FileRegistryServiceProperty.cpp
        src/wrench/services/network_proximity/NetworkProximityDaemon.cpp
        src/wrench/services/network_proximity/NetworkProximityMessage.cpp
        src/wrench/services/network_proximity/NetworkProximityMessage.h
        src/wrench/services/network_proximity/NetworkProximityService.cpp
        src/wrench/services/network_proximity/NetworkProximityServiceMessagePayload.cpp
        src/wrench/services/network_proximity/NetworkProximityServiceProperty.cpp
        src/wrench/services/storage/StorageService.cpp
        src/wrench/services/storage/StorageServiceMessage.cpp
        src/wrench/services/storage/StorageServiceMessage.h
        src/wrench/services/storage/StorageServiceMessagePayload.cpp
        src/wrench/services/storage/StorageServiceProperty.cpp
        src/wrench/services/storage/simple/SimpleStorageService.cpp
        src/wrench/services/storage/simple/SimpleStorageServiceMessagePayload.cpp
        src/wrench/services/storage/simple/SimpleStorageServiceProperty.cpp
        src/wrench/services/storage/storage_helper_classes/FileLocation.cpp
        src/wrench/services/storage/storage_helper_classes/FileTransferThread.cpp
        src/wrench/services/storage/storage_helper_classes/FileTransferThreadMessage.h
        src/wrench/services/storage/storage_helper_classes/LogicalFileSystem.cpp
        src/wrench/simgrid_S4U_util/S4U_Daemon.cpp
        src/wrench/simgrid_S4U_util/S4U_DaemonActor.cpp
        src/wrench/simgrid_S4U_util/S4U_DaemonActor.h
        src/wrench/simgrid_S4U_util/S4U_Mailbox.cpp
        src/wrench/simgrid_S4U_util/S4U_PendingCommunication.cpp
        src/wrench/simgrid_S4U_util/S4U_Simulation.cpp
        src/wrench/simgrid_S4U_util/S4U_VirtualMachine.cpp
        src/wrench/simulation/Simulation.cpp
        src/wrench/simulation/SimulationMessage.cpp
        src/wrench/simulation/SimulationOutput.cpp
        src/wrench/simulation/SimulationTimestamp.cpp
        src/wrench/simulation/SimulationTimestampTypes.cpp
        src/wrench/simulation/SimulationTrace.cpp
        src/wrench/util/MessageManager.cpp
        src/wrench/util/PointerUtil.cpp
        src/wrench/util/PointerUtil.cpp
        src/wrench/util/UnitParser.cpp
        src/wrench/wms/WMS.cpp
        src/wrench/wms/WMSMessage.cpp
        src/wrench/wms/WMSMessage.h
        src/wrench/workflow/Workflow.cpp
        src/wrench/workflow/WorkflowFile.cpp
        src/wrench/workflow/WorkflowTask.cpp
        src/wrench/workflow/execution_events/WorkflowExecutionEvent.cpp
        src/wrench/workflow/failure_causes/ComputeThreadHasDied.cpp
        src/wrench/workflow/failure_causes/FailureCause.cpp
        src/wrench/workflow/failure_causes/FatalFailure.cpp
        src/wrench/workflow/failure_causes/FileAlreadyBeingCopied.cpp
        src/wrench/workflow/failure_causes/FileNotFound.cpp
        src/wrench/workflow/failure_causes/FunctionalityNotAvailable.cpp
        src/wrench/workflow/failure_causes/HostError.cpp
        src/wrench/workflow/failure_causes/InvalidDirectoryPath.cpp
        src/wrench/workflow/failure_causes/JobKilled.cpp
        src/wrench/workflow/failure_causes/JobTimeout.cpp
        src/wrench/workflow/failure_causes/JobTypeNotSupported.cpp
        src/wrench/workflow/failure_causes/NetworkError.cpp
        src/wrench/workflow/failure_causes/NoScratchSpace.cpp
        src/wrench/workflow/failure_causes/NotAllowed.cpp
        src/wrench/workflow/failure_causes/NotEnoughResources.cpp
        src/wrench/workflow/failure_causes/ServiceIsDown.cpp
        src/wrench/workflow/failure_causes/ServiceIsSuspended.cpp
        src/wrench/workflow/failure_causes/StorageServiceNotEnoughSpace.cpp
        src/wrench/workflow/job/PilotJob.cpp
        src/wrench/workflow/job/StandardJob.cpp
        src/wrench/workflow/job/WorkflowJob.cpp
        )

# test files
set(TEST_FILES
        test/main.cpp
        test/include/TestWithFork.h
        test/include/UniqueTmpPathPrefix.h
        test/constructors/simulation_message_constructors/MessageConstructorTest.cpp
        test/constructors/failure_cause_constructor/FailureCauseConstructorTest.cpp
        test/workflow/WorkflowTest.cpp
        test/workflow/WorkflowFileTest.cpp
        test/workflow/WorkflowTaskTest.cpp
        test/workflow/WorkflowLoadFromDAXTest.cpp
        test/workflow/WorkflowLoadFromJSONTest.cpp
        test/compute_services/BareMetalComputeService/BareMetalComputeServiceOneTaskTest.cpp
        test/storage_services/LogicalFileSystem/LogicalFileSystemTest.cpp
        test/storage_services/SimpleStorageService/SimpleStorageServiceFunctionalTest.cpp
        test/storage_services/SimpleStorageService/SimpleStorageServicePerformanceTest.cpp
        test/storage_services/SimpleStorageService/SimpleStorageServiceLimitedConnectionsTest.cpp
        test/storage_services/SimpleStorageService/StorageServiceDeleteRegisterTest.cpp
        test/storage_services/SimpleStorageService/DataMovementManagerCopyRegisterTest.cpp
        test/storage_services/SimpleStorageService/ZeroSizeFileTest.cpp
        test/storage_services/SimpleStorageService/ChunkingTest.cpp
        test/compute_services/BareMetalComputeService/BareMetalComputeServiceTestStandardJobs.cpp
        test/compute_services/BareMetalComputeService/BareMetalComputeServiceTestPilotJobs.cpp
        test/compute_services/BareMetalComputeService/BareMetalComputeServiceSchedulingTest.cpp
        test/compute_services/StandardJobExecutorTest.cpp
        test/compute_services/WorkunitExecutorTest.cpp
        test/compute_services/ComputeThreadTest.cpp
        test/compute_services/BareMetalComputeService/BareMetalComputeServiceResourceInformationTest.cpp
        test/compute_services/BatchService/BatchServiceTest.cpp
        test/compute_services/BatchService/BatchServiceFCFSTest.cpp
        test/compute_services/BatchService/BatchServiceCONSERVATIVEBFTest.cpp
        test/compute_services/BatchService/BatchServiceTraceFileTest.cpp
        test/compute_services/BatchService/BatchServiceOutputCSVFileTest.cpp
        test/compute_services/BatchService/BatchServiceBatschedQueueWaitTimePredictionTest.cpp
        test/compute_services/BatchService/BatchServiceBatschedContiguityTest.cpp
        test/helper_services/HostStateChangeTest.cpp
        test/helper_services/AlarmTest.cpp
        test/wms/WMSTest.cpp
        test/wms/MultipleWMSTest.cpp
        test/wms/WMSOptimizationsTest.cpp
        test/compute_services/VirtualizedClusterService/VirtualizedClusterServiceTest.cpp
        test/compute_services/VirtualizedClusterService/VirtualizedClusterServiceResourceAllocationAlgorithmTest.cpp
        test/network_proximity_services/NetworkProximityTest.cpp
        test/file_registry_services/FileRegistryTest.cpp
        test/wms/JobManagerTest.cpp
        test/simulation/BadPlatformTest.cpp
        test/simulation/SimpleSimulationTest.cpp
        test/simulation/DynamicServiceCreationTest.cpp
        test/simulation/SimulationCommandLineArgumentsTest.cpp
        test/simulation/SimulationLoggingTest.cpp
        test/simulation/simulation_output/SimulationOutputTest.cpp
        test/simulation/simulation_output/SimulationTimestampTaskTest.cpp
        test/simulation/simulation_output/SimulationTimestampFileReadTest.cpp
        test/simulation/simulation_output/SimulationTimestampFileWriteTest.cpp
        test/simulation/simulation_output/SimulationTimestampFileCopyTest.cpp
        test/simulation/simulation_output/SimulationTimestampEnergyTest.cpp
        test/simulation/InvalidXMLTest.cpp
        test/compute_services/ScratchSpaceTest.cpp
        test/simulation/S4U_DaemonTest.cpp
        test/simulation/S4U_VirtualMachineTest.cpp
        test/simulation/S4U_SimulationTest.cpp
        test/pilot_job/CriticalPathSchedulerTest.cpp
        test/misc/PointerUtilTest.cpp
        test/misc/BogusMessageTest.cpp
        examples/simple-example/scheduler/pilot_job/CriticalPathPilotJobScheduler.cpp
        test/compute_services/ScratchSpaceTest.cpp
        test/energy_consumption/EnergyConsumptionTest.cpp
        test/simulation/simulation_output/SimulationDumpJSONTest.cpp
        test/compute_services/HTCondorServiceTest.cpp
        src/wrench/managers/EnergyMeter.cpp
        include/wrench/managers/EnergyMeter.h
        test/simulated_failures/host_failures/ServiceStartReStartTest.cpp
        test/simulated_failures/host_failures/ComprehensiveIntegrationTest.cpp
        test/simulated_failures/host_failures/FailureDetectorHostFailuresTest.cpp
        test/simulated_failures/host_failures/BareMetalComputeServiceHostFailuresTest.cpp
        test/simulated_failures/host_failures/CloudComputeServiceHostFailuresTest.cpp
        test/simulated_failures/host_failures/StandardJobExecutorHostFailuresTest.cpp
        test/simulated_failures/host_failures/NetworkProximityHostFailuresTest.cpp
        test/simulated_failures/host_failures/StorageServiceReStartHostFailuresTest.cpp
        test/simulated_failures/link_failures/FileRegistryLinkFailuresTest.cpp
        test/simulated_failures/link_failures/StorageServiceLinkFailuresTest.cpp
        test/simulated_failures/link_failures/AlarmLinkFailuresTest.cpp
        test/simulated_failures/link_failures/BareMetalComputeServiceLinkFailuresTest.cpp
        test/simulated_failures/link_failures/ComputeThreadLinkFailuresTest.cpp
        test/simulated_failures/link_failures/NetworkProximityLinkFailuresTest.cpp
        test/simulated_failures/failure_test_util/ResourceSwitcher.cpp
        test/simulated_failures/failure_test_util/ResourceSwitcher.h
        test/simulated_failures/failure_test_util/SleeperVictim.cpp
        test/simulated_failures/failure_test_util/SleeperVictim.h
        test/simulated_failures/failure_test_util/ComputerVictim.cpp
        test/simulated_failures/failure_test_util/ComputerVictim.h
        test/simulated_failures/failure_test_util/ResourceRandomRepeatSwitcher.cpp
        test/simulated_failures/failure_test_util/ResourceRandomRepeatSwitcher.h
        test/simulation/S4U_MailboxTest.cpp)

find_library(PUGIXML_LIBRARY NAMES pugixml)
find_library(LEMON_LIBRARY NAMES emon lemon)
find_library(GTEST_LIBRARY NAMES gtest)

add_library(wrench STATIC ${SOURCE_FILES})
set_target_properties(wrench PROPERTIES VERSION ${WRENCH_RELEASE_VERSION})
target_link_libraries(wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})

# wrench version
add_custom_command(TARGET wrench PRE_LINK COMMAND ${CMAKE_COMMAND} -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR} -DWRENCH_RELEASE_VERSION=${WRENCH_RELEASE_VERSION} -P ${CMAKE_HOME_DIRECTORY}/conf/cmake/Version.cmake)

install(TARGETS wrench DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)


# generate unit tests
add_executable(unit_tests EXCLUDE_FROM_ALL ${SOURCE_FILES} ${HEADER_FILES} ${TEST_FILES})
if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(unit_tests ${GTEST_LIBRARY} wrenchpegasusworkflowparser wrench -lpthread -lm ${ZMQ_LIBRARY})
else ()
    target_link_libraries(unit_tests ${GTEST_LIBRARY} wrenchpegasusworkflowparser wrench -lpthread -lm)
endif ()


set_target_properties(unit_tests PROPERTIES COMPILE_FLAGS "-g -O0 --coverage")
set_target_properties(unit_tests PROPERTIES LINK_FLAGS "--coverage")
add_custom_command(TARGET unit_tests COMMAND find . -name *.gcda -delete)


# additional packages
include(${CMAKE_HOME_DIRECTORY}/conf/cmake/DefinePackages.cmake)

# build tools
include(${CMAKE_HOME_DIRECTORY}/conf/cmake/Tools.cmake)

# build examples
include(${CMAKE_HOME_DIRECTORY}/conf/cmake/Examples.cmake)

# build documentation
include(${CMAKE_HOME_DIRECTORY}/conf/cmake/Documentation.cmake)
