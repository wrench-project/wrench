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

#include "wrench/workflow/Workflow.h"
#include "../../src/wrench/services/file_registry/FileRegistryMessage.h"
#include "wrench/services/compute/ComputeServiceMessage.h"
#include "../../src/wrench/services/storage/StorageServiceMessage.h"
#include "../../src/wrench/services/compute/cloud/CloudServiceMessage.h"
#include "../../src/wrench/services/network_proximity/NetworkProximityMessage.h"
#include "wrench/workflow/execution_events/FailureCause.h"

class MessageConstructorTest : public ::testing::Test {
protected:
    MessageConstructorTest() {
      workflow = new wrench::Workflow();
      task = workflow->addTask("task", 1);
      file = workflow->addFile("file", 1);
      storage_service = (wrench::StorageService *)(1234);
      compute_service = (wrench::ComputeService *)(1234);
      workflow_job = (wrench::WorkflowJob *)(1234);
      standard_job = (wrench::StandardJob *)(1234);
      pilot_job = (wrench::PilotJob *)(1234);
      failure_cause = std::make_shared<wrench::FileNotFound>(file, storage_service);
    }

    // data members
    wrench::Workflow *workflow;
    wrench::WorkflowTask *task;
    wrench::WorkflowFile *file;
    wrench::StorageService *storage_service;
    wrench::ComputeService *compute_service;
    wrench::WorkflowJob *workflow_job;
    wrench::StandardJob *standard_job;
    wrench::PilotJob *pilot_job;
    std::shared_ptr<wrench::FileNotFound> failure_cause;
};



TEST_F(MessageConstructorTest, SimulationMessages) {

  wrench::SimulationMessage *msg = nullptr;
  EXPECT_NO_THROW(msg = new wrench::SimulationMessage("name", 666));
  EXPECT_EQ(msg->getName(), "name");
  delete msg;

  EXPECT_THROW(new wrench::SimulationMessage("", 666), std::invalid_argument);
  EXPECT_THROW(new wrench::SimulationMessage("name", -1), std::invalid_argument);

}


TEST_F(MessageConstructorTest, FileRegistryMessages) {

  EXPECT_NO_THROW(new wrench::FileRegistryFileLookupRequestMessage("mailbox_name", file, 666));
  EXPECT_THROW(new wrench::FileRegistryFileLookupRequestMessage("", file, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::FileRegistryFileLookupRequestMessage("mailbox_name", nullptr, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::FileRegistryFileLookupAnswerMessage(file, {}, 666));
  EXPECT_THROW(new wrench::FileRegistryFileLookupAnswerMessage(nullptr, {}, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::FileRegistryRemoveEntryRequestMessage("mailbox_name", file, storage_service, 666));
  EXPECT_THROW(new wrench::FileRegistryRemoveEntryRequestMessage("", file, storage_service, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::FileRegistryRemoveEntryRequestMessage("mailbox_name", nullptr, storage_service, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::FileRegistryRemoveEntryRequestMessage("mailbox_name", file, nullptr, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::FileRegistryRemoveEntryAnswerMessage(true, 666));

  EXPECT_NO_THROW(new wrench::FileRegistryAddEntryRequestMessage("mailbox_name", file, storage_service, 666));
  EXPECT_THROW(new wrench::FileRegistryAddEntryRequestMessage("", file, storage_service, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::FileRegistryAddEntryRequestMessage("mailbox_name", nullptr, storage_service, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::FileRegistryAddEntryRequestMessage("mailbox_name", file, nullptr, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::FileRegistryAddEntryAnswerMessage(666));
}

TEST_F(MessageConstructorTest, ComputeServiceMessages) {

  std::map<std::string, std::string> args;
  args.insert(std::make_pair("a","b"));
  EXPECT_NO_THROW(new wrench::ComputeServiceSubmitStandardJobRequestMessage("mailbox_name", standard_job, args, 666));
  EXPECT_THROW(new wrench::ComputeServiceSubmitStandardJobRequestMessage("", standard_job, args, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::ComputeServiceSubmitStandardJobRequestMessage("mailbox_name", nullptr, args, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::ComputeServiceSubmitStandardJobAnswerMessage(standard_job, compute_service, true, nullptr, 666));
  EXPECT_NO_THROW(new wrench::ComputeServiceSubmitStandardJobAnswerMessage(standard_job, compute_service, false, failure_cause, 666));
  EXPECT_THROW(new wrench::ComputeServiceSubmitStandardJobAnswerMessage(nullptr, compute_service, true, nullptr, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::ComputeServiceSubmitStandardJobAnswerMessage(standard_job, nullptr, true, nullptr, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::ComputeServiceSubmitStandardJobAnswerMessage(standard_job, compute_service, true, failure_cause, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::ComputeServiceSubmitStandardJobAnswerMessage(standard_job, compute_service, false, nullptr, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::ComputeServiceStandardJobDoneMessage(standard_job, compute_service, 666));
  EXPECT_THROW(new wrench::ComputeServiceStandardJobDoneMessage(nullptr, compute_service, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::ComputeServiceStandardJobDoneMessage(standard_job, nullptr, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::ComputeServiceStandardJobFailedMessage(standard_job, compute_service, failure_cause, 666));
  EXPECT_THROW(new wrench::ComputeServiceStandardJobFailedMessage(nullptr, compute_service, failure_cause, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::ComputeServiceStandardJobFailedMessage(standard_job, nullptr, failure_cause, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::ComputeServiceStandardJobFailedMessage(standard_job, compute_service, nullptr, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::ComputeServiceTerminateStandardJobRequestMessage("mailbox_name", standard_job, 666));
  EXPECT_THROW(new wrench::ComputeServiceTerminateStandardJobRequestMessage("", standard_job, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::ComputeServiceTerminateStandardJobRequestMessage("mailbox_name", nullptr, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::ComputeServiceTerminateStandardJobAnswerMessage(standard_job, compute_service, true, nullptr, 666));
  EXPECT_NO_THROW(new wrench::ComputeServiceTerminateStandardJobAnswerMessage(standard_job, compute_service, false, failure_cause, 666));
  EXPECT_THROW(new wrench::ComputeServiceTerminateStandardJobAnswerMessage(nullptr, compute_service, true, nullptr, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::ComputeServiceTerminateStandardJobAnswerMessage(standard_job, nullptr, true, nullptr, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::ComputeServiceTerminateStandardJobAnswerMessage(standard_job, compute_service, true, failure_cause, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::ComputeServiceTerminateStandardJobAnswerMessage(standard_job, compute_service, false, nullptr, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::ComputeServiceSubmitPilotJobRequestMessage("mailbox_name", pilot_job, 666));
  EXPECT_THROW(new wrench::ComputeServiceSubmitPilotJobRequestMessage("", pilot_job, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::ComputeServiceSubmitPilotJobRequestMessage("mailbox_name", nullptr, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::ComputeServiceSubmitPilotJobAnswerMessage(pilot_job, compute_service, true, nullptr, 666));
  EXPECT_NO_THROW(new wrench::ComputeServiceSubmitPilotJobAnswerMessage(pilot_job, compute_service, false, failure_cause, 666));
  EXPECT_THROW(new wrench::ComputeServiceSubmitPilotJobAnswerMessage(nullptr, compute_service, true, nullptr, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::ComputeServiceSubmitPilotJobAnswerMessage(pilot_job, nullptr, true, nullptr, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::ComputeServiceSubmitPilotJobAnswerMessage(pilot_job, compute_service, true, failure_cause, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::ComputeServiceSubmitPilotJobAnswerMessage(pilot_job, compute_service, false, nullptr, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::ComputeServicePilotJobStartedMessage(pilot_job, compute_service, 666));
  EXPECT_THROW(new wrench::ComputeServicePilotJobStartedMessage(nullptr, compute_service, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::ComputeServicePilotJobStartedMessage(pilot_job, nullptr, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::ComputeServicePilotJobExpiredMessage(pilot_job, compute_service, 666));
  EXPECT_THROW(new wrench::ComputeServicePilotJobExpiredMessage(nullptr, compute_service, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::ComputeServicePilotJobExpiredMessage(pilot_job, nullptr, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::ComputeServicePilotJobFailedMessage(pilot_job, compute_service, 666));
  EXPECT_THROW(new wrench::ComputeServicePilotJobFailedMessage(nullptr, compute_service, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::ComputeServicePilotJobFailedMessage(pilot_job, nullptr, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::ComputeServiceTerminatePilotJobRequestMessage("mailbox_name", pilot_job, 666));
  EXPECT_THROW(new wrench::ComputeServiceTerminatePilotJobRequestMessage("", pilot_job, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::ComputeServiceTerminatePilotJobRequestMessage("mailbox_name", nullptr, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::ComputeServiceTerminatePilotJobAnswerMessage(pilot_job, compute_service, true, nullptr, 666));
  EXPECT_NO_THROW(new wrench::ComputeServiceTerminatePilotJobAnswerMessage(pilot_job, compute_service, false, failure_cause, 666));
  EXPECT_THROW(new wrench::ComputeServiceTerminatePilotJobAnswerMessage(nullptr, compute_service, true, nullptr, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::ComputeServiceTerminatePilotJobAnswerMessage(pilot_job, nullptr, true, nullptr, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::ComputeServiceTerminatePilotJobAnswerMessage(pilot_job, compute_service, true, failure_cause, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::ComputeServiceTerminatePilotJobAnswerMessage(pilot_job, compute_service, false, nullptr, 666), std::invalid_argument);


  EXPECT_NO_THROW(new wrench::ComputeServiceResourceInformationRequestMessage("mailbox_name", 666));
  EXPECT_THROW(new wrench::ComputeServiceResourceInformationRequestMessage("", 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::ComputeServiceResourceInformationAnswerMessage({std::make_pair("something", std::vector<double>({2.3, 4.5}))}, 666));

}


TEST_F(MessageConstructorTest, CloudServiceMessages) {

  EXPECT_NO_THROW(new wrench::CloudServiceGetExecutionHostsRequestMessage("mailbox_name", 600));
  EXPECT_THROW(new wrench::CloudServiceGetExecutionHostsRequestMessage("", 666), std::invalid_argument);

  std::vector<std::string> arg;
  arg.push_back("aaa");
  EXPECT_NO_THROW(new wrench::CloudServiceGetExecutionHostsAnswerMessage(arg, 600));

  std::map<std::string, std::string> plist;
  EXPECT_NO_THROW(new wrench::CloudServiceCreateVMRequestMessage("mailbox_name", "host", "host", true, true, 42, 10, plist, 666));
  EXPECT_THROW(new wrench::CloudServiceCreateVMRequestMessage("", "host", "host", true, true, 42, 0, plist, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::CloudServiceCreateVMRequestMessage("mailbox_name", "", "host", true, true, 42, 0, plist, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::CloudServiceCreateVMRequestMessage("mailbox_name", "host", "", true, true, 42, 0, plist, 666), std::invalid_argument);
}



TEST_F(MessageConstructorTest, StorageServiceMessages) {
  EXPECT_NO_THROW(new wrench::StorageServiceFreeSpaceRequestMessage("mailbox_name", 666));
  EXPECT_THROW(new wrench::StorageServiceFreeSpaceRequestMessage("", 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::StorageServiceFreeSpaceAnswerMessage(0.1, 666));
  EXPECT_THROW(new wrench::StorageServiceFreeSpaceAnswerMessage(-0.1, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::StorageServiceFileLookupRequestMessage("mailbox_name", file, 666));
  EXPECT_THROW(new wrench::StorageServiceFileLookupRequestMessage("", file, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::StorageServiceFileLookupRequestMessage("mailbox_name", nullptr, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::StorageServiceFileLookupAnswerMessage(file, true, 666));
  EXPECT_THROW(new wrench::StorageServiceFileLookupAnswerMessage(nullptr, true, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::StorageServiceFileDeleteRequestMessage("mailbox_name", file, 666));
  EXPECT_THROW(new wrench::StorageServiceFileDeleteRequestMessage("", file, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::StorageServiceFileDeleteRequestMessage("mailbox_name", nullptr, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::StorageServiceFileDeleteAnswerMessage(file, storage_service, true, nullptr, 666));
  EXPECT_NO_THROW(new wrench::StorageServiceFileDeleteAnswerMessage(file, storage_service, false, failure_cause, 666));
  EXPECT_THROW(new wrench::StorageServiceFileDeleteAnswerMessage(nullptr, storage_service, true, failure_cause, 666),
               std::invalid_argument);
  EXPECT_THROW(new wrench::StorageServiceFileDeleteAnswerMessage(file, nullptr, true, failure_cause, 666),
               std::invalid_argument);
  EXPECT_THROW(new wrench::StorageServiceFileDeleteAnswerMessage(file, storage_service, false, nullptr, 666),
               std::invalid_argument);
  EXPECT_THROW(new wrench::StorageServiceFileDeleteAnswerMessage(file, storage_service, true, failure_cause, 666),
               std::invalid_argument);

  EXPECT_NO_THROW(new wrench::StorageServiceFileCopyRequestMessage("mailbox_name", file, storage_service, 666));
  EXPECT_THROW(new wrench::StorageServiceFileCopyRequestMessage("", file, storage_service, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::StorageServiceFileCopyRequestMessage("mailbox_name", nullptr, storage_service, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::StorageServiceFileCopyRequestMessage("mailbox_name", file, nullptr, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, storage_service, true, nullptr, 666));
  EXPECT_NO_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, storage_service, false, failure_cause, 666));
  EXPECT_THROW(new wrench::StorageServiceFileCopyAnswerMessage(nullptr, storage_service, true, nullptr, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, nullptr, true, nullptr, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, storage_service, true, failure_cause, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, storage_service, false, nullptr, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::StorageServiceFileWriteRequestMessage("mailbox_name", file, 666));
  EXPECT_THROW(new wrench::StorageServiceFileWriteRequestMessage("", file, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::StorageServiceFileWriteRequestMessage("mailbox_name", nullptr, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::StorageServiceFileWriteAnswerMessage(file, storage_service, true, nullptr, "mailbox_name", 666));
  EXPECT_NO_THROW(new wrench::StorageServiceFileWriteAnswerMessage(file, storage_service, false, failure_cause, "mailbox_name", 666));
  EXPECT_THROW(new wrench::StorageServiceFileWriteAnswerMessage(nullptr, storage_service, true, nullptr, "mailbox_name", 666), std::invalid_argument);
  EXPECT_THROW(new wrench::StorageServiceFileWriteAnswerMessage(file, nullptr, true, nullptr, "mailbox_name", 666), std::invalid_argument);
  EXPECT_THROW(new wrench::StorageServiceFileWriteAnswerMessage(file, storage_service, true, nullptr, "", 666), std::invalid_argument);
  EXPECT_THROW(new wrench::StorageServiceFileWriteAnswerMessage(file, storage_service, true, failure_cause, "mailbox_name", 666), std::invalid_argument);
  EXPECT_THROW(new wrench::StorageServiceFileWriteAnswerMessage(file, storage_service, false, nullptr, "mailbox_name", 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::StorageServiceFileReadRequestMessage("mailbox_name", "mailbox_name", file, 666));
  EXPECT_THROW(new wrench::StorageServiceFileReadRequestMessage("", "mailbox_name", file, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::StorageServiceFileReadRequestMessage("mailbox_name", "", file, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::StorageServiceFileReadRequestMessage("", "mailbox_name", nullptr, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::StorageServiceFileReadAnswerMessage(file, storage_service, true, nullptr, 666));
  EXPECT_NO_THROW(new wrench::StorageServiceFileReadAnswerMessage(file, storage_service, false, failure_cause, 666));
  EXPECT_THROW(new wrench::StorageServiceFileReadAnswerMessage(nullptr, storage_service, true, nullptr, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::StorageServiceFileReadAnswerMessage(file, nullptr, true, nullptr, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::StorageServiceFileReadAnswerMessage(file, storage_service, true, failure_cause, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::StorageServiceFileReadAnswerMessage(file, storage_service, false, nullptr, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::StorageServiceFileContentMessage(file));
  EXPECT_THROW(new wrench::StorageServiceFileContentMessage(nullptr), std::invalid_argument);
}


TEST_F(MessageConstructorTest, NetworkProximityMessages) {

  EXPECT_NO_THROW(new wrench::NetworkProximityLookupRequestMessage("mailbox_name", std::make_pair("a","b"), 666));
  EXPECT_THROW(new wrench::NetworkProximityLookupRequestMessage("", std::make_pair("a","b"), 666), std::invalid_argument);
  EXPECT_THROW(new wrench::NetworkProximityLookupRequestMessage("mailbox_name", std::make_pair("","b"), 666), std::invalid_argument);
  EXPECT_THROW(new wrench::NetworkProximityLookupRequestMessage("mailbox_name", std::make_pair("a",""), 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::NetworkProximityLookupAnswerMessage(std::make_pair("a","b"), 1.0, 666));
  EXPECT_THROW(new wrench::NetworkProximityLookupAnswerMessage(std::make_pair("","b"), 1.0, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::NetworkProximityLookupAnswerMessage(std::make_pair("a",""), 1.0, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::NetworkProximityComputeAnswerMessage(std::make_pair("a","b"), 1.0, 666));
  EXPECT_THROW(new wrench::NetworkProximityComputeAnswerMessage(std::make_pair("","b"), 1.0, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::NetworkProximityComputeAnswerMessage(std::make_pair("a",""), 1.0, 666), std::invalid_argument);

}


