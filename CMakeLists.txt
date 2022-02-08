cmake_minimum_required(VERSION 3.2)
message(STATUS "Cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")

project(wrench CXX)

# Make Release the default build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_definitions("-Wall -Wno-unused-variable -Wno-unused-private-field")
if (ENABLE_BATSCHED)
    add_definitions(-DENABLE_BATSCHED)
endif ()
if (ENABLE_MESSAGE_MANAGER)
    add_definitions(-DMESSAGE_MANAGER)
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/conf/cmake/")
find_package(Boost REQUIRED)
find_package(SimGrid REQUIRED)

set(CMAKE_CXX_STANDARD 14)

# build the version number
set(WRENCH_VERSION_MAJOR "1")
set(WRENCH_VERSION_MINOR "11")
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

include_directories(src/wrench/ include/ ${SimGrid_INCLUDE_DIR} ${Boost_INCLUDE_DIR} /usr/include /usr/local/include /opt/simgrid/include)


# For MacOS's MacPorts
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} /opt/local/lib)
include_directories(/opt/local/include)

# library header files
set(HEADER_FILES
        include/wrench-dev.h
        include/wrench.h
        include/wrench/exceptions/ExecutionException.h
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
        include/wrench/services/compute/bare_metal/BareMetalComputeServiceOneShot.h
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
        include/wrench/services/helper_services/action_execution_service/ActionExecutionServiceProperty.h
        include/wrench/services/helper_services/action_execution_service/ActionExecutionServiceMessage.h
        include/wrench/services/helper_services/action_execution_service/ActionExecutionService.h
        include/wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeService.h
        include/wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeServiceMessagePayload.h
        include/wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeServiceProperty.h
        include/wrench/services/helper_services/action_executor/ActionExecutor.h
        include/wrench/services/helper_services/action_executor/ActionExecutorMessage.h
        include/wrench/services/file_registry/FileRegistryService.h
        include/wrench/services/file_registry/FileRegistryServiceMessagePayload.h
        include/wrench/services/file_registry/FileRegistryServiceProperty.h
        include/wrench/services/helper_services/alarm/Alarm.h
        include/wrench/services/helper_services/host_state_change_detector/HostStateChangeDetector.h
        include/wrench/services/helper_services/host_state_change_detector/HostStateChangeDetectorMessage.h
        include/wrench/services/helper_services/host_state_change_detector/HostStateChangeDetectorProperty.h
        include/wrench/services/helper_services/service_termination_detector/ServiceTerminationDetector.h
        include/wrench/services/helper_services/service_termination_detector/ServiceTerminationDetectorMessage.h
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
        include/wrench/services/memory/Block.h
        include/wrench/services/memory/MemoryManager.h
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
        include/wrench/workflow/Workflow.h
        include/wrench/workflow/WorkflowTask.h
        include/wrench/execution_events/ExecutionEvent.h
        include/wrench/execution_events/CompoundJobCompletedEvent.h
        include/wrench/execution_events/CompoundJobFailedEvent.h
        include/wrench/execution_events/StandardJobCompletedEvent.h
        include/wrench/execution_events/StandardJobFailedEvent.h
        include/wrench/execution_events/PilotJobStartedEvent.h
        include/wrench/execution_events/PilotJobExpiredEvent.h
        include/wrench/execution_events/FileCopyCompletedEvent.h
        include/wrench/execution_events/FileCopyFailedEvent.h
        include/wrench/execution_events/TimerEvent.h
        include/wrench/failure_causes/ComputeThreadHasDied.h
        include/wrench/failure_causes/FailureCause.h
        include/wrench/failure_causes/SomeActionsHaveFailed.h
        include/wrench/failure_causes/FatalFailure.h
        include/wrench/failure_causes/FileAlreadyBeingCopied.h
        include/wrench/failure_causes/FileNotFound.h
        include/wrench/failure_causes/FunctionalityNotAvailable.h
        include/wrench/failure_causes/HostError.h
        include/wrench/failure_causes/InvalidDirectoryPath.h
        include/wrench/failure_causes/JobKilled.h
        include/wrench/failure_causes/JobTimeout.h
        include/wrench/failure_causes/NetworkError.h
        include/wrench/failure_causes/NoScratchSpace.h
        include/wrench/failure_causes/NotAllowed.h
        include/wrench/failure_causes/NotEnoughResources.h
        include/wrench/failure_causes/ServiceIsDown.h
        include/wrench/failure_causes/ServiceIsSuspended.h
        include/wrench/failure_causes/StorageServiceNotEnoughSpace.h
        include/wrench/job/PilotJob.h
        include/wrench/job/StandardJob.h
        include/wrench/job/CompoundJob.h
        include/wrench/action/Action.h
        include/wrench/action/SleepAction.h
        include/wrench/action/CustomAction.h
        include/wrench/action/FileReadAction.h
        include/wrench/action/FileWriteAction.h
        include/wrench/action/FileCopyAction.h
        include/wrench/action/FileDeleteAction.h
        include/wrench/action/FileRegistryAction.h
        include/wrench/action/FileRegistryAddEntryAction.h
        include/wrench/action/FileRegistryDeleteEntryAction.h
        include/wrench/action/ComputeAction.h
        include/wrench/job/Job.h
        include/wrench/workflow/DagOfTasks.h
        include/wrench/workflow/parallel_model/ParallelModel.h
        include/wrench/workflow/parallel_model/AmdahlParallelModel.h
        include/wrench/workflow/parallel_model/ConstantEfficiencyParallelModel.h
        include/wrench/workflow/parallel_model/CustomParallelModel.h
        include/wrench/execution_controller/ExecutionController.h
        include/wrench/execution_controller/ExecutionControllerMessage.h
        include/wrench/data_file/DataFile.h
        )

# source files
set(SOURCE_FILES
        include/wrench/services/compute/batch/batch_schedulers/BatchScheduler.h
        include/wrench/services/compute/batch/batch_schedulers/homegrown/HomegrownBatchScheduler.h
        include/wrench/services/storage/storage_helpers/FileTransferThread.h
        include/wrench/services/storage/storage_helpers/LogicalFileSystem.h
        src/wrench/services/helper_services/alarm/Alarm.cpp
        src/wrench/services/helper_services/host_state_change_detector/HostStateChangeDetector.cpp
        src/wrench/services/helper_services/host_state_change_detector/HostStateChangeDetectorMessage.cpp
        src/wrench/services/helper_services/host_state_change_detector/HostStateChangeDetectorProperty.cpp
        src/wrench/services/helper_services/service_termination_detector/ServiceTerminationDetector.cpp
        src/wrench/services/helper_services/service_termination_detector/ServiceTerminationDetectorMessage.cpp
        src/wrench/services/helper_services/action_execution_service/ActionExecutionService.cpp
        src/wrench/services/helper_services/action_execution_service/ActionExecutionServiceProperty.cpp
        src/wrench/services/helper_services/action_execution_service/ActionExecutionServiceMessage.cpp
        src/wrench/services/helper_services/compute_thread/ComputeThread.cpp
        src/wrench/services/helper_services/compute_thread/ComputeThreadMessage.cpp
        include/wrench/services/helper_services/compute_thread/ComputeThread.h
        include/wrench/services/helper_services/compute_thread/ComputeThreadMessage.h
        src/wrench/services/helper_services/action_executor/ActionExecutor.cpp
        src/wrench/services/helper_services/action_executor/ActionExecutorMessage.cpp
        src/wrench/logging/TerminalOutput.cpp
        src/wrench/managers/DataMovementManager.cpp
        src/wrench/services/metering/EnergyMeterService.cpp
        src/wrench/services/metering/BandwidthMeterService.cpp
        src/wrench/managers/JobManager.cpp
        src/wrench/managers/JobManagerMessage.cpp
        include/wrench/managers/JobManagerMessage.h
        src/wrench/services/Service.cpp
        src/wrench/services/ServiceMessage.cpp
        src/wrench/services/ServiceMessagePayload.cpp
        src/wrench/services/ServiceProperty.cpp
        src/wrench/services/compute/ComputeService.cpp
        src/wrench/services/compute/ComputeServiceMessage.cpp
        src/wrench/services/compute/ComputeServiceMessagePayload.cpp
        src/wrench/services/compute/ComputeServiceProperty.cpp
        src/wrench/services/compute/bare_metal/BareMetalComputeService.cpp
        src/wrench/services/compute/bare_metal/BareMetalComputeServiceOneShot.cpp
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
        include/wrench/services/compute/batch/batch_schedulers/batsched/BatschedBatchScheduler.h
        src/wrench/services/compute/batch/batch_schedulers/homegrown/HomegrownBatchScheduler.cpp
        include/wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf/BatchJobSet.h
        src/wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf/ConservativeBackfillingBatchScheduler.cpp
        include/wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf/ConservativeBackfillingBatchScheduler.h
        src/wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf/NodeAvailabilityTimeLine.cpp
        include/wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf/NodeAvailabilityTimeLine.h
        include/wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf_core_level/BatchJobSetCoreLevel.h
        src/wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf_core_level/ConservativeBackfillingBatchSchedulerCoreLevel.cpp
        include/wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf_core_level/ConservativeBackfillingBatchSchedulerCoreLevel.h
        src/wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf_core_level/CoreAvailabilityTimeLine.cpp
        include/wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf_core_level/CoreAvailabilityTimeLine.h
        src/wrench/services/compute/batch/batch_schedulers/homegrown/fcfs/FCFSBatchScheduler.cpp
        include/wrench/services/compute/batch/batch_schedulers/homegrown/fcfs/FCFSBatchScheduler.h
        src/wrench/services/compute/batch/workload_helper_classes/TraceFileLoader.cpp
        src/wrench/services/compute/batch/workload_helper_classes/WorkloadTraceFileReplayer.cpp
        src/wrench/services/compute/batch/workload_helper_classes/WorkloadTraceFileReplayerEventReceiver.cpp
        src/wrench/services/compute/cloud/CloudComputeService.cpp
        src/wrench/services/compute/cloud/CloudComputeServiceMessage.cpp
        include/wrench/services/compute/cloud/CloudComputeServiceMessage.h
        src/wrench/services/compute/cloud/CloudComputeServiceMessagePayload.cpp
        src/wrench/services/compute/cloud/CloudComputeServiceProperty.cpp
        src/wrench/services/compute/htcondor/HTCondorCentralManagerService.cpp
        src/wrench/services/compute/htcondor/HTCondorCentralManagerServiceMessage.cpp
        src/wrench/services/compute/htcondor/HTCondorCentralManagerServiceMessagePayload.cpp
        src/wrench/services/compute/htcondor/HTCondorComputeService.cpp
        src/wrench/services/compute/htcondor/HTCondorComputeServiceProperty.cpp
        src/wrench/services/compute/htcondor/HTCondorNegotiatorService.cpp
        src/wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeService.cpp
        src/wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeServiceMessage.cpp
        include/wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeServiceMessage.h
        src/wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeServiceMessagePayload.cpp
        src/wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeServiceProperty.cpp
        src/wrench/services/file_registry/FileRegistryMessage.cpp
        include/wrench/services/file_registry/FileRegistryMessage.h
        src/wrench/services/file_registry/FileRegistryService.cpp
        src/wrench/services/file_registry/FileRegistryServiceMessagePayload.cpp
        src/wrench/services/file_registry/FileRegistryServiceProperty.cpp
        src/wrench/services/network_proximity/NetworkProximityDaemon.cpp
        src/wrench/services/network_proximity/NetworkProximityMessage.cpp
        include/wrench/services/network_proximity/NetworkProximityMessage.h
        src/wrench/services/network_proximity/NetworkProximityService.cpp
        src/wrench/services/network_proximity/NetworkProximityServiceMessagePayload.cpp
        src/wrench/services/network_proximity/NetworkProximityServiceProperty.cpp
        src/wrench/services/storage/StorageService.cpp
        src/wrench/services/storage/StorageServiceMessage.cpp
        include/wrench/services/storage/StorageServiceMessage.h
        src/wrench/services/storage/StorageServiceMessagePayload.cpp
        src/wrench/services/storage/StorageServiceProperty.cpp
        src/wrench/services/storage/simple/SimpleStorageService.cpp
        src/wrench/services/storage/simple/SimpleStorageServiceMessagePayload.cpp
        src/wrench/services/storage/simple/SimpleStorageServiceProperty.cpp
        src/wrench/services/storage/storage_helper_classes/FileLocation.cpp
        src/wrench/services/storage/storage_helper_classes/FileTransferThread.cpp
        include/wrench/services/storage/storage_helpers/FileTransferThreadMessage.h
        src/wrench/services/storage/storage_helper_classes/LogicalFileSystem.cpp
        src/wrench/services/memory/Block.cpp
        src/wrench/services/memory/MemoryManager.cpp
        src/wrench/simgrid_S4U_util/S4U_Daemon.cpp
        src/wrench/simgrid_S4U_util/S4U_DaemonActor.cpp
        include/wrench/simgrid_S4U_util/S4U_DaemonActor.h
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
        src/wrench/util/UnitParser.cpp
        src/wrench/workflow/Workflow.cpp
        src/wrench/workflow/DagOfTasks.cpp
        src/wrench/workflow/WorkflowTask.cpp
        src/wrench/workflow/parallel_model/ParallelModel.cpp
        src/wrench/workflow/parallel_model/AmdahlParallelModel.cpp
        src/wrench/workflow/parallel_model/ConstantEfficiencyParallelModel.cpp
        src/wrench/workflow/parallel_model/CustomParallelModel.cpp
        src/wrench/execution_events/ExecutionEvent.cpp
        src/wrench/failure_causes/ComputeThreadHasDied.cpp
        src/wrench/failure_causes/FailureCause.cpp
        src/wrench/failure_causes/SomeActionsHaveFailed.cpp
        src/wrench/failure_causes/FatalFailure.cpp
        src/wrench/failure_causes/FileAlreadyBeingCopied.cpp
        src/wrench/failure_causes/FileNotFound.cpp
        src/wrench/failure_causes/FunctionalityNotAvailable.cpp
        src/wrench/failure_causes/HostError.cpp
        src/wrench/failure_causes/InvalidDirectoryPath.cpp
        src/wrench/failure_causes/JobKilled.cpp
        src/wrench/failure_causes/JobTimeout.cpp
        src/wrench/failure_causes/NetworkError.cpp
        src/wrench/failure_causes/NoScratchSpace.cpp
        src/wrench/failure_causes/NotAllowed.cpp
        src/wrench/failure_causes/NotEnoughResources.cpp
        src/wrench/failure_causes/ServiceIsDown.cpp
        src/wrench/failure_causes/ServiceIsSuspended.cpp
        src/wrench/failure_causes/StorageServiceNotEnoughSpace.cpp
        src/wrench/job/PilotJob.cpp
        src/wrench/job/StandardJob.cpp
        src/wrench/job/CompoundJob.cpp
        src/wrench/action/Action.cpp
        src/wrench/action/SleepAction.cpp
        src/wrench/action/CustomAction.cpp
        src/wrench/action/FileReadAction.cpp
        src/wrench/action/FileWriteAction.cpp
        src/wrench/action/FileCopyAction.cpp
        src/wrench/action/FileDeleteAction.cpp
        src/wrench/action/FileRegistryAction.cpp
        src/wrench/action/ComputeAction.cpp
        src/wrench/job/Job.cpp
        src/wrench/execution_controller/ExecutionController.cpp
        src/wrench/execution_controller/ExecutionControllerMessage.cpp
        src/wrench/data_file/DataFile.cpp
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
        test/workflow/WorkflowParallelModelTest.cpp
        test/workflow/WorkflowLoadFromDAXTest.cpp
        test/workflow/WorkflowLoadFromJSONTest.cpp
        test/services/memory_manager_service/MemoryManagerTest.cpp
        test/services/compute_services/bare_metal_compound_jobs/BareMetalComputeServiceOneActionTests.cpp
        test/services/compute_services/bare_metal_compound_jobs/BareMetalComputeServiceMultiActionTests.cpp
        test/services/compute_services/bare_metal_compound_jobs/BareMetalComputeServiceMultiJobTests.cpp
        test/services/compute_services/bare_metal_standard_jobs/BareMetalComputeServiceOneTaskTest.cpp
        test/services/storage_services/LogicalFileSystem/LogicalFileSystemTest.cpp
        test/services/storage_services/SimpleStorageService/SimpleStorageServiceFunctionalTest.cpp
        test/services/storage_services/SimpleStorageService/SimpleStorageServicePerformanceTest.cpp
        test/services/storage_services/SimpleStorageService/SimpleStorageServiceLimitedConnectionsTest.cpp
        test/services/storage_services/SimpleStorageService/StorageServiceDeleteRegisterTest.cpp
        test/services/storage_services/SimpleStorageService/DataMovementManagerCopyRegisterTest.cpp
        test/services/storage_services/SimpleStorageService/ZeroSizeFileTest.cpp
        test/services/storage_services/SimpleStorageService/ChunkingTest.cpp
        test/services/compute_services/bare_metal_standard_jobs/BareMetalComputeServiceTestStandardJobs.cpp
        test/services/compute_services/bare_metal_standard_jobs/BareMetalComputeServiceTestPilotJobs.cpp
        test/services/compute_services/bare_metal_standard_jobs/BareMetalComputeServiceSchedulingTest.cpp
        test/services/helper_services/action_executor/ComputeActionExecutorTest.cpp
        test/services/helper_services/action_executor/SleepActionExecutorTest.cpp
        test/services/helper_services/action_executor/FileReadActionExecutorTest.cpp
        test/services/helper_services/action_executor/CustomActionExecutorTest.cpp
        test/services/helper_services/action_executor/FileDeleteActionExecutorTest.cpp
        test/services/helper_services/action_executor/FileCopyActionExecutorTest.cpp
        test/services/helper_services/action_executor/FileWriteActionExecutorTest.cpp
        test/services/helper_services/action_executor/KillFailureActionExecutorTest.cpp
        test/services/helper_services/action_execution_service/ActionExecutionServiceTest.cpp
        test/services/helper_services/ComputeThreadTest.cpp
        test/services/compute_services/bare_metal_standard_jobs/BareMetalComputeServiceResourceInformationTest.cpp
        test/services/compute_services/batch_compound_jobs/BatchComputeServiceOneActionTests.cpp
        test/services/compute_services/batch_standard_and_pilot_jobs/HomeGrownTimeLineTest.cpp
        test/services/compute_services/batch_standard_and_pilot_jobs/BatchServiceTest.cpp
        test/services/compute_services/batch_standard_and_pilot_jobs/BatchServiceFCFSTest.cpp
        test/services/compute_services/batch_standard_and_pilot_jobs/BatchServiceCONSERVATIVEBFTest.cpp
        test/services/compute_services/batch_standard_and_pilot_jobs/BatchServiceTraceFileTest.cpp
        test/services/compute_services/batch_standard_and_pilot_jobs/BatchServiceOutputCSVFileTest.cpp
        test/services/compute_services/batch_standard_and_pilot_jobs/BatchServiceBatschedQueueWaitTimePredictionTest.cpp
        test/services/compute_services/batch_standard_and_pilot_jobs/BatchServiceBatschedContiguityTest.cpp
        test/services/helper_services/HostStateChangeTest.cpp
        test/services/helper_services/AlarmTest.cpp
        test/wms/WMSTest.cpp
        test/wms/MultipleWMSTest.cpp
        test/services/compute_services/virtualized_cluster/VirtualizedClusterServiceTest.cpp
        test/services/compute_services/virtualized_cluster/VirtualizedClusterServiceResourceAllocationAlgorithmTest.cpp
        test/services/network_proximity_service/NetworkProximityTest.cpp
        test/services/file_registry_service/FileRegistryTest.cpp
        test/wms/JobManagerTest.cpp
        test/simulation/BadPlatformTest.cpp
        test/simulation/SimpleSimulationTest.cpp
        test/simulation/DynamicServiceCreationTest.cpp
        test/simulation/SimulationCommandLineArgumentsTest.cpp
        test/simulation/SimulationLoggingTest.cpp
        test/simulation/simulation_output/SimulationOutputTest.cpp
        test/simulation/simulation_output/SimulationTimestampDiskReadWriteTest.cpp
        test/simulation/simulation_output/SimulationTimestampTaskTest.cpp
        test/simulation/simulation_output/SimulationTimestampFileReadTest.cpp
        test/simulation/simulation_output/SimulationTimestampFileWriteTest.cpp
        test/simulation/simulation_output/SimulationTimestampFileCopyTest.cpp
        test/simulation/simulation_output/SimulationTimestampEnergyTest.cpp
        test/simulation/InvalidXMLTest.cpp
        test/services/compute_services/bare_metal_standard_jobs/ScratchSpaceTest.cpp
        test/simulation/S4U_DaemonTest.cpp
        test/simulation/S4U_VirtualMachineTest.cpp
        test/simulation/S4U_SimulationTest.cpp
        test/misc/PointerUtilTest.cpp
        test/misc/BogusMessageTest.cpp
        test/services/compute_services/bare_metal_standard_jobs/ScratchSpaceTest.cpp
        test/energy_consumption/EnergyConsumptionTest.cpp
        test/simulation/simulation_output/SimulationDumpJSONTest.cpp
        test/services/compute_services/htcondor/HTCondorServiceTest.cpp
        src/wrench/services/metering/EnergyMeterService.cpp
        src/wrench/services/metering/BandwidthMeterService.cpp
        include/wrench/services/metering/EnergyMeterService.h
        include/wrench/services/metering/BandwidthMeterService.h
        test/services/bandwidth_meter_service/BandwidthMeterServiceTest.cpp
        test/simulated_failures/host_failures/ServiceStartReStartTest.cpp
        test/simulated_failures/host_failures/ComprehensiveIntegrationTest.cpp
        test/simulated_failures/host_failures/FailureDetectorHostFailuresTest.cpp
        test/simulated_failures/host_failures/BareMetalComputeServiceHostFailuresTest.cpp
        test/simulated_failures/host_failures/CloudComputeServiceHostFailuresTest.cpp
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
        test/simulation/S4U_MailboxTest.cpp src/wrench/action/Action.cpp
        )

find_library(PUGIXML_LIBRARY NAMES pugixml)
find_library(GTEST_LIBRARY NAMES gtest)

add_library(wrench STATIC ${SOURCE_FILES})
set_target_properties(wrench PROPERTIES VERSION ${WRENCH_RELEASE_VERSION})
target_link_libraries(wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY})

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
