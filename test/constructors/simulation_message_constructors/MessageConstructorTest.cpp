/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <map>
#include <wrench/services/compute/batch/BatchServiceMessage.h>

#include "wrench/workflow/Workflow.h"
#include "services/file_registry/FileRegistryMessage.h"
#include "wrench/services/compute/ComputeServiceMessage.h"
#include "services/storage/StorageServiceMessage.h"
#include "services/compute/cloud/CloudServiceMessage.h"
#include "services/compute/virtualized_cluster/VirtualizedClusterServiceMessage.h"
#include "services/network_proximity/NetworkProximityMessage.h"
#include "wrench/workflow/execution_events/FailureCause.h"

class MessageConstructorTest : public ::testing::Test {
protected:
    MessageConstructorTest() {
      workflow = new wrench::Workflow();
      workflow_unique_ptr = std::unique_ptr<wrench::Workflow>(workflow);
      task = workflow->addTask("task", 1, 1, 1, 1.0, 0);
      file = workflow->addFile("file", 1);
      storage_service = (wrench::StorageService *)(1234);
      compute_service = (wrench::ComputeService *)(1234);
      network_proximity_service = (wrench::NetworkProximityService *)(1234);
      network_proximity_daemon = (wrench::NetworkProximityDaemon *)(1234);
      workflow_job = (wrench::WorkflowJob *)(1234);
      standard_job = (wrench::StandardJob *)(1234);
      batch_job = (wrench::BatchJob *)(1234);
      pilot_job = (wrench::PilotJob *)(1234);
      file_copy_start_time_stamp = new wrench::SimulationTimestampFileCopyStart(file, storage_service, "dir", storage_service, "dir");
      failure_cause = std::make_shared<wrench::FileNotFound>(file, storage_service);
      file_copy_start_time_stamp = new wrench::SimulationTimestampFileCopyStart(file, storage_service, "dir", storage_service, "dir");
    }

    // data members
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr;
    wrench::Workflow *workflow;
    wrench::WorkflowTask *task;
    wrench::WorkflowFile *file;
    wrench::StorageService *storage_service;
    wrench::ComputeService *compute_service;
    wrench::NetworkProximityService *network_proximity_service;
    wrench::NetworkProximityDaemon *network_proximity_daemon;
    wrench::WorkflowJob *workflow_job;
    wrench::StandardJob *standard_job;
    wrench::BatchJob *batch_job;
    wrench::PilotJob *pilot_job;
    wrench::SimulationTimestampFileCopyStart *file_copy_start_time_stamp;
    std::shared_ptr<wrench::FileNotFound> failure_cause;
};



TEST_F(MessageConstructorTest, SimulationMessages) {

  wrench::SimulationMessage *msg = nullptr;
  ASSERT_NO_THROW(msg = new wrench::SimulationMessage("name", 666));
  ASSERT_EQ(msg->getName(), "name");
  delete msg;

  ASSERT_THROW(new wrench::SimulationMessage("", 666), std::invalid_argument);
  ASSERT_THROW(new wrench::SimulationMessage("name", -1), std::invalid_argument);

}

TEST_F(MessageConstructorTest, FileRegistryMessages) {

  ASSERT_NO_THROW(new wrench::FileRegistryFileLookupRequestMessage("mailbox", file, 666));
  ASSERT_THROW(new wrench::FileRegistryFileLookupRequestMessage("", file, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::FileRegistryFileLookupRequestMessage("mailbox", nullptr, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::FileRegistryFileLookupAnswerMessage(file, {}, 666));
  ASSERT_THROW(new wrench::FileRegistryFileLookupAnswerMessage(nullptr, {}, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::FileRegistryRemoveEntryRequestMessage("mailbox", file, storage_service, 666));
  ASSERT_THROW(new wrench::FileRegistryRemoveEntryRequestMessage("", file, storage_service, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::FileRegistryRemoveEntryRequestMessage("mailbox", nullptr, storage_service, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::FileRegistryRemoveEntryRequestMessage("mailbox", file, nullptr, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::FileRegistryRemoveEntryAnswerMessage(true, 666));

  ASSERT_NO_THROW(new wrench::FileRegistryAddEntryRequestMessage("mailbox", file, storage_service, 666));
  ASSERT_THROW(new wrench::FileRegistryAddEntryRequestMessage("", file, storage_service, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::FileRegistryAddEntryRequestMessage("mailbox", nullptr, storage_service, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::FileRegistryAddEntryRequestMessage("mailbox", file, nullptr, 666), std::invalid_argument);

    ASSERT_NO_THROW(new wrench::FileRegistryAddEntryAnswerMessage(666));

    ASSERT_NO_THROW(new wrench::FileRegistryFileLookupByProximityRequestMessage("mailbox", file, "reference_host",
                                                                                network_proximity_service, 666));
    ASSERT_THROW(new wrench::FileRegistryFileLookupByProximityRequestMessage("", file, "reference_host",
                                                                             network_proximity_service, 666),
                 std::invalid_argument);
    ASSERT_THROW(new wrench::FileRegistryFileLookupByProximityRequestMessage("", nullptr, "reference_host",
                                                                             network_proximity_service, 666),
                 std::invalid_argument);
    ASSERT_THROW(
            new wrench::FileRegistryFileLookupByProximityRequestMessage("", file, "", network_proximity_service, 666),
            std::invalid_argument);
    ASSERT_THROW(new wrench::FileRegistryFileLookupByProximityRequestMessage("", file, "reference_host", nullptr, 666),
                 std::invalid_argument);

    ASSERT_NO_THROW(new wrench::FileRegistryFileLookupByProximityAnswerMessage(file, "reference_host", {{}}, 666));
    ASSERT_THROW(new wrench::FileRegistryFileLookupByProximityAnswerMessage(nullptr, "reference_host", {{}}, 666), std::invalid_argument);
    ASSERT_THROW(new wrench::FileRegistryFileLookupByProximityAnswerMessage(file, "", {{}}, 666), std::invalid_argument);
}

TEST_F(MessageConstructorTest, ComputeServiceMessages) {

  std::map<std::string, std::string> args;
  args.insert(std::make_pair("a","b"));
  ASSERT_NO_THROW(new wrench::ComputeServiceSubmitStandardJobRequestMessage("mailbox", standard_job, args, 666));
  ASSERT_THROW(new wrench::ComputeServiceSubmitStandardJobRequestMessage("", standard_job, args, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::ComputeServiceSubmitStandardJobRequestMessage("mailbox", nullptr, args, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::ComputeServiceSubmitStandardJobAnswerMessage(standard_job, compute_service, true, nullptr, 666));
  ASSERT_NO_THROW(new wrench::ComputeServiceSubmitStandardJobAnswerMessage(standard_job, compute_service, false, failure_cause, 666));
  ASSERT_THROW(new wrench::ComputeServiceSubmitStandardJobAnswerMessage(nullptr, compute_service, true, nullptr, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::ComputeServiceSubmitStandardJobAnswerMessage(standard_job, nullptr, true, nullptr, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::ComputeServiceSubmitStandardJobAnswerMessage(standard_job, compute_service, true, failure_cause, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::ComputeServiceSubmitStandardJobAnswerMessage(standard_job, compute_service, false, nullptr, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::ComputeServiceStandardJobDoneMessage(standard_job, compute_service, 666));
  ASSERT_THROW(new wrench::ComputeServiceStandardJobDoneMessage(nullptr, compute_service, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::ComputeServiceStandardJobDoneMessage(standard_job, nullptr, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::ComputeServiceStandardJobFailedMessage(standard_job, compute_service, failure_cause, 666));
  ASSERT_THROW(new wrench::ComputeServiceStandardJobFailedMessage(nullptr, compute_service, failure_cause, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::ComputeServiceStandardJobFailedMessage(standard_job, nullptr, failure_cause, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::ComputeServiceStandardJobFailedMessage(standard_job, compute_service, nullptr, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::ComputeServiceTerminateStandardJobRequestMessage("mailbox", standard_job, 666));
  ASSERT_THROW(new wrench::ComputeServiceTerminateStandardJobRequestMessage("", standard_job, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::ComputeServiceTerminateStandardJobRequestMessage("mailbox", nullptr, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::ComputeServiceTerminateStandardJobAnswerMessage(standard_job, compute_service, true, nullptr, 666));
  ASSERT_NO_THROW(new wrench::ComputeServiceTerminateStandardJobAnswerMessage(standard_job, compute_service, false, failure_cause, 666));
  ASSERT_THROW(new wrench::ComputeServiceTerminateStandardJobAnswerMessage(nullptr, compute_service, true, nullptr, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::ComputeServiceTerminateStandardJobAnswerMessage(standard_job, nullptr, true, nullptr, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::ComputeServiceTerminateStandardJobAnswerMessage(standard_job, compute_service, true, failure_cause, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::ComputeServiceTerminateStandardJobAnswerMessage(standard_job, compute_service, false, nullptr, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::ComputeServiceSubmitPilotJobRequestMessage("mailbox", pilot_job, args, 666));
  ASSERT_THROW(new wrench::ComputeServiceSubmitPilotJobRequestMessage("", pilot_job, args, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::ComputeServiceSubmitPilotJobRequestMessage("mailbox", nullptr, args, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::ComputeServiceSubmitPilotJobAnswerMessage(pilot_job, compute_service, true, nullptr, 666));
  ASSERT_NO_THROW(new wrench::ComputeServiceSubmitPilotJobAnswerMessage(pilot_job, compute_service, false, failure_cause, 666));
  ASSERT_THROW(new wrench::ComputeServiceSubmitPilotJobAnswerMessage(nullptr, compute_service, true, nullptr, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::ComputeServiceSubmitPilotJobAnswerMessage(pilot_job, nullptr, true, nullptr, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::ComputeServiceSubmitPilotJobAnswerMessage(pilot_job, compute_service, true, failure_cause, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::ComputeServiceSubmitPilotJobAnswerMessage(pilot_job, compute_service, false, nullptr, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::ComputeServicePilotJobStartedMessage(pilot_job, compute_service, 666));
  ASSERT_THROW(new wrench::ComputeServicePilotJobStartedMessage(nullptr, compute_service, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::ComputeServicePilotJobStartedMessage(pilot_job, nullptr, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::ComputeServicePilotJobExpiredMessage(pilot_job, compute_service, 666));
  ASSERT_THROW(new wrench::ComputeServicePilotJobExpiredMessage(nullptr, compute_service, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::ComputeServicePilotJobExpiredMessage(pilot_job, nullptr, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::ComputeServicePilotJobFailedMessage(pilot_job, compute_service, 666));
  ASSERT_THROW(new wrench::ComputeServicePilotJobFailedMessage(nullptr, compute_service, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::ComputeServicePilotJobFailedMessage(pilot_job, nullptr, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::ComputeServiceTerminatePilotJobRequestMessage("mailbox", pilot_job, 666));
  ASSERT_THROW(new wrench::ComputeServiceTerminatePilotJobRequestMessage("", pilot_job, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::ComputeServiceTerminatePilotJobRequestMessage("mailbox", nullptr, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::ComputeServiceTerminatePilotJobAnswerMessage(pilot_job, compute_service, true, nullptr, 666));
  ASSERT_NO_THROW(new wrench::ComputeServiceTerminatePilotJobAnswerMessage(pilot_job, compute_service, false, failure_cause, 666));
  ASSERT_THROW(new wrench::ComputeServiceTerminatePilotJobAnswerMessage(nullptr, compute_service, true, nullptr, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::ComputeServiceTerminatePilotJobAnswerMessage(pilot_job, nullptr, true, nullptr, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::ComputeServiceTerminatePilotJobAnswerMessage(pilot_job, compute_service, true, failure_cause, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::ComputeServiceTerminatePilotJobAnswerMessage(pilot_job, compute_service, false, nullptr, 666), std::invalid_argument);


  ASSERT_NO_THROW(new wrench::ComputeServiceResourceInformationRequestMessage("mailbox", 666));
  ASSERT_THROW(new wrench::ComputeServiceResourceInformationRequestMessage("", 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::ComputeServiceResourceInformationAnswerMessage({std::make_pair("something", std::map<std::string, double>({{"aa", 2.3}, {"bb", 4.5}}))}, 666));

//  ASSERT_NO_THROW(new wrench::ComputeServiceInformationMessage(workflow_job, "info", 666));
}


TEST_F(MessageConstructorTest, CloudServiceMessages) {

  ASSERT_NO_THROW(new wrench::CloudServiceGetExecutionHostsRequestMessage("mailbox", 600));
  ASSERT_THROW(new wrench::CloudServiceGetExecutionHostsRequestMessage("", 666), std::invalid_argument);

  std::vector<std::string> arg;
  arg.push_back("aaa");
  ASSERT_NO_THROW(new wrench::CloudServiceGetExecutionHostsAnswerMessage(arg, 600));

  std::map<std::string, std::string> property_list;
  std::map<std::string, std::string> messagepayload_list;
  ASSERT_NO_THROW(new wrench::CloudServiceCreateVMRequestMessage("mailbox", "host", 42, 10, property_list, messagepayload_list, 666));
  ASSERT_THROW(new wrench::CloudServiceCreateVMRequestMessage("", "host", 42, 0, property_list, messagepayload_list, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::CloudServiceCreateVMRequestMessage("mailbox", "", 42, 0, property_list, messagepayload_list, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::VirtualizedClusterServiceMigrateVMRequestMessage("mailbox", "host", "host", 666));
  ASSERT_THROW(new wrench::VirtualizedClusterServiceMigrateVMRequestMessage("", "host", "host", 666), std::invalid_argument);
  ASSERT_THROW(new wrench::VirtualizedClusterServiceMigrateVMRequestMessage("mailbox", "", "host", 666), std::invalid_argument);
  ASSERT_THROW(new wrench::VirtualizedClusterServiceMigrateVMRequestMessage("mailbox", "host", "", 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::CloudServiceShutdownVMRequestMessage("mailbox", "vm", 666));
  ASSERT_THROW(new wrench::CloudServiceShutdownVMRequestMessage("", "vm", 666), std::invalid_argument);
  ASSERT_THROW(new wrench::CloudServiceShutdownVMRequestMessage("mailbox", "", 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::CloudServiceShutdownVMAnswerMessage(true, 666));

  ASSERT_NO_THROW(new wrench::CloudServiceStartVMRequestMessage("mailbox", "vm", 666));
  ASSERT_THROW(new wrench::CloudServiceStartVMRequestMessage("", "vm", 666), std::invalid_argument);
  ASSERT_THROW(new wrench::CloudServiceStartVMRequestMessage("mailbox", "", 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::CloudServiceStartVMAnswerMessage(true, 666));

  ASSERT_NO_THROW(new wrench::CloudServiceSuspendVMRequestMessage("mailbox", "vm", 666));
  ASSERT_THROW(new wrench::CloudServiceSuspendVMRequestMessage("", "vm", 666), std::invalid_argument);
  ASSERT_THROW(new wrench::CloudServiceSuspendVMRequestMessage("mailbox", "", 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::CloudServiceSuspendVMAnswerMessage(true, 666));

  ASSERT_NO_THROW(new wrench::CloudServiceResumeVMRequestMessage("mailbox", "vm", 666));
  ASSERT_THROW(new wrench::CloudServiceResumeVMRequestMessage("", "vm", 666), std::invalid_argument);
  ASSERT_THROW(new wrench::CloudServiceResumeVMRequestMessage("mailbox", "", 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::CloudServiceResumeVMAnswerMessage(true, 666));


}



TEST_F(MessageConstructorTest, StorageServiceMessages) {
  ASSERT_NO_THROW(new wrench::StorageServiceFreeSpaceRequestMessage("mailbox", 666));
  ASSERT_THROW(new wrench::StorageServiceFreeSpaceRequestMessage("", 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::StorageServiceFreeSpaceAnswerMessage(0.1, 666));
  ASSERT_THROW(new wrench::StorageServiceFreeSpaceAnswerMessage(-0.1, 666), std::invalid_argument);

  std::string root_dir = "/";
  ASSERT_NO_THROW(new wrench::StorageServiceFileLookupRequestMessage("mailbox", file, root_dir, 666));
  ASSERT_THROW(new wrench::StorageServiceFileLookupRequestMessage("", file, root_dir, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::StorageServiceFileLookupRequestMessage("mailbox", nullptr, root_dir, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::StorageServiceFileLookupAnswerMessage(file, true, 666));
  ASSERT_THROW(new wrench::StorageServiceFileLookupAnswerMessage(nullptr, true, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::StorageServiceFileDeleteRequestMessage("mailbox", file, root_dir, 666));
  ASSERT_THROW(new wrench::StorageServiceFileDeleteRequestMessage("", file, root_dir, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::StorageServiceFileDeleteRequestMessage("mailbox", nullptr, root_dir, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::StorageServiceFileDeleteAnswerMessage(file, storage_service, true, nullptr, 666));
  ASSERT_NO_THROW(new wrench::StorageServiceFileDeleteAnswerMessage(file, storage_service, false, failure_cause, 666));
  ASSERT_THROW(new wrench::StorageServiceFileDeleteAnswerMessage(nullptr, storage_service, true, failure_cause, 666),
               std::invalid_argument);
  ASSERT_THROW(new wrench::StorageServiceFileDeleteAnswerMessage(file, nullptr, true, failure_cause, 666),
               std::invalid_argument);
  ASSERT_THROW(new wrench::StorageServiceFileDeleteAnswerMessage(file, storage_service, false, nullptr, 666),
               std::invalid_argument);
  ASSERT_THROW(new wrench::StorageServiceFileDeleteAnswerMessage(file, storage_service, true, failure_cause, 666),
               std::invalid_argument);

  ASSERT_NO_THROW(new wrench::StorageServiceFileCopyRequestMessage("mailbox", file, storage_service, root_dir, storage_service, root_dir, nullptr, file_copy_start_time_stamp, 666));
  ASSERT_THROW(new wrench::StorageServiceFileCopyRequestMessage("", file, storage_service, root_dir, storage_service, root_dir, nullptr, file_copy_start_time_stamp, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::StorageServiceFileCopyRequestMessage("mailbox", nullptr, storage_service, root_dir, storage_service, root_dir, nullptr, file_copy_start_time_stamp, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::StorageServiceFileCopyRequestMessage("mailbox", file, nullptr, root_dir, storage_service, root_dir, nullptr, file_copy_start_time_stamp, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::StorageServiceFileCopyRequestMessage("mailbox", file, storage_service, root_dir, nullptr, root_dir, nullptr, file_copy_start_time_stamp, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::StorageServiceFileCopyRequestMessage("mailbox", file, storage_service, root_dir, storage_service, root_dir, nullptr, nullptr, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, storage_service, "dir", nullptr, false, true, nullptr, 666));
  ASSERT_NO_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, storage_service, "dir", nullptr, false, false, failure_cause, 666));
  ASSERT_THROW(new wrench::StorageServiceFileCopyAnswerMessage(nullptr, storage_service, "dir", nullptr, false, true, nullptr, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, nullptr, "dir", nullptr, false, true, nullptr, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, storage_service, "", nullptr, false, true, nullptr, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, storage_service, "dir", nullptr, true, true, nullptr, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, storage_service, "dir", nullptr, false, false, nullptr, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, storage_service, "dir", nullptr, false, true, failure_cause, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::StorageServiceFileWriteRequestMessage("mailbox", file, root_dir, 666));
  ASSERT_THROW(new wrench::StorageServiceFileWriteRequestMessage("", file, root_dir, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::StorageServiceFileWriteRequestMessage("mailbox", nullptr, root_dir, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::StorageServiceFileWriteAnswerMessage(file, storage_service, true, nullptr, "mailbox", 666));
  ASSERT_NO_THROW(new wrench::StorageServiceFileWriteAnswerMessage(file, storage_service, false, failure_cause, "mailbox", 666));
  ASSERT_THROW(new wrench::StorageServiceFileWriteAnswerMessage(nullptr, storage_service, true, nullptr, "mailbox", 666), std::invalid_argument);
  ASSERT_THROW(new wrench::StorageServiceFileWriteAnswerMessage(file, nullptr, true, nullptr, "mailbox", 666), std::invalid_argument);
  ASSERT_THROW(new wrench::StorageServiceFileWriteAnswerMessage(file, storage_service, true, nullptr, "", 666), std::invalid_argument);
  ASSERT_THROW(new wrench::StorageServiceFileWriteAnswerMessage(file, storage_service, true, failure_cause, "mailbox", 666), std::invalid_argument);
  ASSERT_THROW(new wrench::StorageServiceFileWriteAnswerMessage(file, storage_service, false, nullptr, "mailbox", 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::StorageServiceFileReadRequestMessage("mailbox", "mailbox", file, root_dir, 666));
  ASSERT_THROW(new wrench::StorageServiceFileReadRequestMessage("", "mailbox", file, root_dir, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::StorageServiceFileReadRequestMessage("mailbox", "", file, root_dir, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::StorageServiceFileReadRequestMessage("", "mailbox", nullptr, root_dir, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::StorageServiceFileReadAnswerMessage(file, storage_service, true, nullptr, 666));
  ASSERT_NO_THROW(new wrench::StorageServiceFileReadAnswerMessage(file, storage_service, false, failure_cause, 666));
  ASSERT_THROW(new wrench::StorageServiceFileReadAnswerMessage(nullptr, storage_service, true, nullptr, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::StorageServiceFileReadAnswerMessage(file, nullptr, true, nullptr, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::StorageServiceFileReadAnswerMessage(file, storage_service, true, failure_cause, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::StorageServiceFileReadAnswerMessage(file, storage_service, false, nullptr, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::StorageServiceFileContentMessage(file));
  ASSERT_THROW(new wrench::StorageServiceFileContentMessage(nullptr), std::invalid_argument);
}

TEST_F(MessageConstructorTest, NetworkProximityMessages) {

  ASSERT_NO_THROW(new wrench::NetworkProximityLookupRequestMessage("mailbox", std::make_pair("a","b"), 666));
  ASSERT_THROW(new wrench::NetworkProximityLookupRequestMessage("", std::make_pair("a","b"), 666), std::invalid_argument);
  ASSERT_THROW(new wrench::NetworkProximityLookupRequestMessage("mailbox", std::make_pair("","b"), 666), std::invalid_argument);
  ASSERT_THROW(new wrench::NetworkProximityLookupRequestMessage("mailbox", std::make_pair("a",""), 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::NetworkProximityLookupAnswerMessage(std::make_pair("a","b"), 1.0, 1.0, 666));
  ASSERT_THROW(new wrench::NetworkProximityLookupAnswerMessage(std::make_pair("","b"), 1.0, 1.0, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::NetworkProximityLookupAnswerMessage(std::make_pair("a",""), 1.0, 1.0, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::NetworkProximityComputeAnswerMessage(std::make_pair("a","b"), 1.0, 666));
  ASSERT_THROW(new wrench::NetworkProximityComputeAnswerMessage(std::make_pair("","b"), 1.0, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::NetworkProximityComputeAnswerMessage(std::make_pair("a",""), 1.0, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::NextContactDaemonRequestMessage(network_proximity_daemon, 666));
  ASSERT_THROW(new wrench::NextContactDaemonRequestMessage(nullptr, 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::CoordinateLookupRequestMessage("mailbox", "requested_host", 666));
  ASSERT_THROW(new wrench::CoordinateLookupRequestMessage("", "requested_host", 666), std::invalid_argument);
  ASSERT_THROW(new wrench::CoordinateLookupRequestMessage("mailbox", "", 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::CoordinateLookupAnswerMessage("requested_host", std::make_pair(1.0,1.0), 1.0, 666));
  ASSERT_THROW(new wrench::CoordinateLookupAnswerMessage("", std::make_pair(1.0, 1.0), 1.0, 666), std::invalid_argument);
}



TEST_F(MessageConstructorTest, BatchServiceMessages) {

//  ASSERT_NO_THROW(new wrench::BatchSimulationBeginsToSchedulerMessage("mailbox", "foo", 666));
//  ASSERT_THROW(new wrench::BatchSimulationBeginsToSchedulerMessage("mailbox", "", 666), std::invalid_argument);

//  ASSERT_NO_THROW(new wrench::BatchJobSubmissionToSchedulerMessage("mailbox", workflow_job, "foo", 666));
//  ASSERT_THROW(new wrench::BatchJobSubmissionToSchedulerMessage("", workflow_job, "foo", 666), std::invalid_argument);
//  ASSERT_THROW(new wrench::BatchJobSubmissionToSchedulerMessage("mailbox", nullptr, "foo", 666), std::invalid_argument);
//  ASSERT_THROW(new wrench::BatchJobSubmissionToSchedulerMessage("mailbox", workflow_job, "", 666), std::invalid_argument);

//  ASSERT_NO_THROW(new wrench::BatchJobReplyFromSchedulerMessage("reply", 666));

//  ASSERT_NO_THROW(new wrench::BatchSchedReadyMessage("mailbox", 666));
//  ASSERT_THROW(new wrench::BatchSchedReadyMessage("", 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::BatchExecuteJobFromBatSchedMessage("mailbox", "string", 666));
  ASSERT_THROW(new wrench::BatchExecuteJobFromBatSchedMessage("", "string", 666), std::invalid_argument);
  ASSERT_THROW(new wrench::BatchExecuteJobFromBatSchedMessage("mailbox", "", 666), std::invalid_argument);

  ASSERT_NO_THROW(new wrench::BatchQueryAnswerMessage(1.0, 666));

  ASSERT_NO_THROW(new wrench::BatchServiceJobRequestMessage("mailbox", batch_job, 666));
  ASSERT_THROW(new wrench::BatchServiceJobRequestMessage("mailbox", nullptr, 666), std::invalid_argument);
  ASSERT_THROW(new wrench::BatchServiceJobRequestMessage("", batch_job, 666), std::invalid_argument);
  ASSERT_NO_THROW(new wrench::AlarmJobTimeOutMessage(batch_job, 666));
  ASSERT_THROW(new wrench::AlarmJobTimeOutMessage(nullptr, 666), std::invalid_argument);


//  ASSERT_NO_THROW(new wrench::AlarmNotifyBatschedMessage("job_id", 666));
}

