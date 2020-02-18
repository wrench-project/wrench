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
#include <wrench/services/compute/batch/BatchComputeServiceMessage.h>

#include "wrench/workflow/WorkflowFile.h"
#include "wrench/workflow/Workflow.h"
#include "services/file_registry/FileRegistryMessage.h"
#include "wrench/services/compute/ComputeServiceMessage.h"
#include "services/storage/StorageServiceMessage.h"
#include "services/compute/cloud/CloudComputeServiceMessage.h"
#include "services/compute/virtualized_cluster/VirtualizedClusterComputeServiceMessage.h"
#include "services/network_proximity/NetworkProximityMessage.h"
#include "wrench/workflow/execution_events/FailureCause.h"

class MessageConstructorTest : public ::testing::Test {
protected:

    ~MessageConstructorTest() {
    }

    MessageConstructorTest() {
        workflow = new wrench::Workflow();
        workflow_unique_ptr = std::unique_ptr<wrench::Workflow>(workflow);
        task = workflow->addTask("task", 1, 1, 1, 1.0, 0);
        file = workflow->addFile("file", 1);
        storage_service = std::shared_ptr<wrench::StorageService>((wrench::StorageService *)(1234), [](void *ptr){});
        location = std::shared_ptr<wrench::FileLocation>((wrench::FileLocation *)1234);
        compute_service = std::shared_ptr<wrench::ComputeService>((wrench::ComputeService *)(1234), [](void *ptr){});
        network_proximity_service = std::shared_ptr<wrench::NetworkProximityService>((wrench::NetworkProximityService *)(1234), [](void *ptr){});
        network_proximity_daemon = std::shared_ptr<wrench::NetworkProximityDaemon>((wrench::NetworkProximityDaemon *)(1234), [](void *ptr){});
        workflow_job = (wrench::WorkflowJob *)(1234);
        standard_job = (wrench::StandardJob *)(1234);
        batch_job = (wrench::BatchJob *)(1234);
        pilot_job = (wrench::PilotJob *)(1234);
        failure_cause = std::shared_ptr<wrench::FileNotFound>(new wrench::FileNotFound(file, location), [](void *ptr){});
    }


    // data members
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr;
    wrench::Workflow *workflow;
    wrench::WorkflowTask *task;
    wrench::WorkflowFile *file;
    std::shared_ptr<wrench::StorageService> storage_service;
    std::shared_ptr<wrench::FileLocation> location;
    std::shared_ptr<wrench::ComputeService> compute_service;
    std::shared_ptr<wrench::NetworkProximityService> network_proximity_service;
    std::shared_ptr<wrench::NetworkProximityDaemon> network_proximity_daemon;
    wrench::WorkflowJob *workflow_job;
    wrench::StandardJob *standard_job;
    wrench::BatchJob *batch_job;
    wrench::PilotJob *pilot_job;
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

    ASSERT_NO_THROW(new wrench::FileRegistryRemoveEntryRequestMessage("mailbox", file, location, 666));
    ASSERT_THROW(new wrench::FileRegistryRemoveEntryRequestMessage("", file, location, 666), std::invalid_argument);
    ASSERT_THROW(new wrench::FileRegistryRemoveEntryRequestMessage("mailbox", nullptr, location, 666), std::invalid_argument);
    ASSERT_THROW(new wrench::FileRegistryRemoveEntryRequestMessage("mailbox", file, nullptr, 666), std::invalid_argument);

    ASSERT_NO_THROW(new wrench::FileRegistryRemoveEntryAnswerMessage(true, 666));

    ASSERT_NO_THROW(new wrench::FileRegistryAddEntryRequestMessage("mailbox", file, location, 666));
    ASSERT_THROW(new wrench::FileRegistryAddEntryRequestMessage("", file, location, 666), std::invalid_argument);
    ASSERT_THROW(new wrench::FileRegistryAddEntryRequestMessage("mailbox", nullptr, location, 666), std::invalid_argument);
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


TEST_F(MessageConstructorTest, CloudComputeServiceMessages) {

    ASSERT_NO_THROW(new wrench::CloudComputeServiceGetExecutionHostsRequestMessage("mailbox", 600));
    ASSERT_THROW(new wrench::CloudComputeServiceGetExecutionHostsRequestMessage("", 666), std::invalid_argument);

    std::vector<std::string> arg;
    arg.push_back("aaa");
    ASSERT_NO_THROW(new wrench::CloudComputeServiceGetExecutionHostsAnswerMessage(arg, 600));

    std::map<std::string, std::string> property_list;
    std::map<std::string, std::string> messagepayload_list;
    ASSERT_NO_THROW(new wrench::CloudComputeServiceCreateVMRequestMessage("mailbox", 42, 10, "stuff", {}, {}, 666));
    ASSERT_THROW(
            new wrench::CloudComputeServiceCreateVMRequestMessage("", 42, 0, "stuff", {}, {}, 666), std::invalid_argument);

    ASSERT_NO_THROW(new wrench::VirtualizedClusterComputeServiceMigrateVMRequestMessage("mailbox", "host", "host", 666));
    ASSERT_THROW(new wrench::VirtualizedClusterComputeServiceMigrateVMRequestMessage("", "host", "host", 666),
                 std::invalid_argument);
    ASSERT_THROW(new wrench::VirtualizedClusterComputeServiceMigrateVMRequestMessage("mailbox", "", "host", 666),
                 std::invalid_argument);
    ASSERT_THROW(new wrench::VirtualizedClusterComputeServiceMigrateVMRequestMessage("mailbox", "host", "", 666),
                 std::invalid_argument);

    ASSERT_NO_THROW(new wrench::CloudComputeServiceShutdownVMRequestMessage("mailbox", "vm", 666));
    ASSERT_THROW(new wrench::CloudComputeServiceShutdownVMRequestMessage("", "vm", 666), std::invalid_argument);
    ASSERT_THROW(new wrench::CloudComputeServiceShutdownVMRequestMessage("mailbox", "", 666), std::invalid_argument);

    ASSERT_NO_THROW(new wrench::CloudComputeServiceShutdownVMAnswerMessage(true, nullptr, 666));

    ASSERT_NO_THROW(new wrench::CloudComputeServiceStartVMRequestMessage("mailbox", "vm", "host", 666));
    ASSERT_THROW(new wrench::CloudComputeServiceStartVMRequestMessage("", "vm", "host", 666), std::invalid_argument);
    ASSERT_THROW(new wrench::CloudComputeServiceStartVMRequestMessage("mailbox", "", "host", 666), std::invalid_argument);

    ASSERT_NO_THROW(new wrench::CloudComputeServiceStartVMAnswerMessage(true, nullptr, nullptr, 666));

    ASSERT_NO_THROW(new wrench::CloudComputeServiceSuspendVMRequestMessage("mailbox", "vm", 666));
    ASSERT_THROW(new wrench::CloudComputeServiceSuspendVMRequestMessage("", "vm", 666), std::invalid_argument);
    ASSERT_THROW(new wrench::CloudComputeServiceSuspendVMRequestMessage("mailbox", "", 666), std::invalid_argument);

    ASSERT_NO_THROW(new wrench::CloudComputeServiceSuspendVMAnswerMessage(true, nullptr, 666));

    ASSERT_NO_THROW(new wrench::CloudComputeServiceResumeVMRequestMessage("mailbox", "vm", 666));
    ASSERT_THROW(new wrench::CloudComputeServiceResumeVMRequestMessage("", "vm", 666), std::invalid_argument);
    ASSERT_THROW(new wrench::CloudComputeServiceResumeVMRequestMessage("mailbox", "", 666), std::invalid_argument);

    ASSERT_NO_THROW(new wrench::CloudComputeServiceResumeVMAnswerMessage(true, nullptr, 666));

    ASSERT_NO_THROW(new wrench::CloudComputeServiceDestroyVMRequestMessage("mailbox", "vm", 666));
    ASSERT_THROW(new wrench::CloudComputeServiceDestroyVMRequestMessage("", "vm", 666), std::invalid_argument);
    ASSERT_THROW(new wrench::CloudComputeServiceDestroyVMRequestMessage("mailbox", "", 666), std::invalid_argument);

    ASSERT_NO_THROW(new wrench::CloudComputeServiceDestroyVMAnswerMessage(true, nullptr, 666));

}


TEST_F(MessageConstructorTest, StorageServiceMessages) {
    ASSERT_NO_THROW(new wrench::StorageServiceFreeSpaceRequestMessage("mailbox", 666));
    ASSERT_THROW(new wrench::StorageServiceFreeSpaceRequestMessage("", 666), std::invalid_argument);

    ASSERT_NO_THROW(new wrench::StorageServiceFreeSpaceAnswerMessage({{"/", 1000}, {"/foo/", 2000}}, 666));
    ASSERT_THROW(new wrench::StorageServiceFreeSpaceAnswerMessage({{"/", -10.0}}, 666), std::invalid_argument);

    std::string root_dir = "/";
    ASSERT_NO_THROW(new wrench::StorageServiceFileLookupRequestMessage("mailbox", file, location, 666));
    ASSERT_THROW(new wrench::StorageServiceFileLookupRequestMessage("", file, location, 666), std::invalid_argument);
    ASSERT_THROW(new wrench::StorageServiceFileLookupRequestMessage("mailbox", nullptr, location, 666), std::invalid_argument);
    ASSERT_THROW(new wrench::StorageServiceFileLookupRequestMessage("mailbox", file, nullptr, 666), std::invalid_argument);

    ASSERT_NO_THROW(new wrench::StorageServiceFileLookupAnswerMessage(file, true, 666));
    ASSERT_THROW(new wrench::StorageServiceFileLookupAnswerMessage(nullptr, true, 666), std::invalid_argument);

    ASSERT_NO_THROW(new wrench::StorageServiceFileDeleteRequestMessage("mailbox", file, location, 666));
    ASSERT_THROW(new wrench::StorageServiceFileDeleteRequestMessage("", file, location, 666), std::invalid_argument);
    ASSERT_THROW(new wrench::StorageServiceFileDeleteRequestMessage("mailbox", nullptr, location, 666), std::invalid_argument);
    ASSERT_THROW(new wrench::StorageServiceFileDeleteRequestMessage("mailbox", file, nullptr, 666), std::invalid_argument);

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

    ASSERT_NO_THROW(new wrench::StorageServiceFileCopyRequestMessage("mailbox", file, location, location, nullptr, 666));
    ASSERT_THROW(new wrench::StorageServiceFileCopyRequestMessage("", file, location, location, nullptr, 666), std::invalid_argument);
    ASSERT_THROW(new wrench::StorageServiceFileCopyRequestMessage("mailbox", nullptr, location, location, nullptr, 666), std::invalid_argument);
    ASSERT_THROW(new wrench::StorageServiceFileCopyRequestMessage("mailbox", file, nullptr, location, nullptr, 666), std::invalid_argument);
    ASSERT_THROW(new wrench::StorageServiceFileCopyRequestMessage("mailbox", file, location, nullptr, nullptr, 666), std::invalid_argument);

    ASSERT_THROW(new wrench::StorageServiceFileCopyAnswerMessage(nullptr, location, location, nullptr, false, true, nullptr, 666), std::invalid_argument);
    ASSERT_NO_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, location, location, nullptr, false, true, nullptr, 666));
    ASSERT_NO_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, location, location, nullptr, false, false, failure_cause, 666));
    ASSERT_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, nullptr, nullptr, nullptr, false, true, nullptr, 666), std::invalid_argument);
    ASSERT_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, location, nullptr, nullptr, false, true, nullptr, 666), std::invalid_argument);
    ASSERT_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, location, nullptr, nullptr, true, true, nullptr, 666), std::invalid_argument);
    ASSERT_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, location, nullptr, nullptr, false, false, nullptr, 666), std::invalid_argument);
    ASSERT_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, location, nullptr, nullptr, false, true, failure_cause, 666), std::invalid_argument);

    ASSERT_NO_THROW(new wrench::StorageServiceFileWriteRequestMessage("mailbox", file, location, 10, 666));
    ASSERT_THROW(new wrench::StorageServiceFileWriteRequestMessage("", nullptr, location, 10, 666), std::invalid_argument);
    ASSERT_THROW(new wrench::StorageServiceFileWriteRequestMessage("mailbox", file, nullptr, 10, 666), std::invalid_argument);

    ASSERT_NO_THROW(new wrench::StorageServiceFileWriteAnswerMessage(file, location, true, nullptr, "mailbox", 666));
    ASSERT_NO_THROW(new wrench::StorageServiceFileWriteAnswerMessage(file, location, false, failure_cause, "", 666));
    ASSERT_THROW(new wrench::StorageServiceFileWriteAnswerMessage(nullptr, location, true, nullptr, "mailbox", 666), std::invalid_argument);
    ASSERT_THROW(new wrench::StorageServiceFileWriteAnswerMessage(file, nullptr, true, nullptr, "mailbox", 666), std::invalid_argument);
    ASSERT_THROW(new wrench::StorageServiceFileWriteAnswerMessage(file, location, true, nullptr, "", 666), std::invalid_argument);
    ASSERT_THROW(new wrench::StorageServiceFileWriteAnswerMessage(file, location, true, failure_cause, "mailbox", 666), std::invalid_argument);
    ASSERT_THROW(new wrench::StorageServiceFileWriteAnswerMessage(file, location, false, nullptr, "mailbox", 666), std::invalid_argument);

    ASSERT_NO_THROW(new wrench::StorageServiceFileReadRequestMessage("mailbox", "mailbox", file, location, 10, 666));
    ASSERT_THROW(new wrench::StorageServiceFileReadRequestMessage("", "mailbox", file, location, 10, 666), std::invalid_argument);
    ASSERT_THROW(new wrench::StorageServiceFileReadRequestMessage("mailbox", "", file, location, 10, 666), std::invalid_argument);
    ASSERT_THROW(new wrench::StorageServiceFileReadRequestMessage("mailbox", "mailbox", nullptr, location, 10, 666), std::invalid_argument);
    ASSERT_THROW(new wrench::StorageServiceFileReadRequestMessage("mailbox", "mailbox", file, nullptr, 10, 666), std::invalid_argument);

    ASSERT_NO_THROW(new wrench::StorageServiceFileReadAnswerMessage(file, location, true, nullptr, 666));
    ASSERT_NO_THROW(new wrench::StorageServiceFileReadAnswerMessage(file, location, false, failure_cause, 666));
    ASSERT_THROW(new wrench::StorageServiceFileReadAnswerMessage(nullptr, location, true, nullptr, 666), std::invalid_argument);
    ASSERT_THROW(new wrench::StorageServiceFileReadAnswerMessage(file, nullptr, true, nullptr, 666), std::invalid_argument);
    ASSERT_THROW(new wrench::StorageServiceFileReadAnswerMessage(file, location, true, failure_cause, 666), std::invalid_argument);
    ASSERT_THROW(new wrench::StorageServiceFileReadAnswerMessage(file, location, false, nullptr, 666), std::invalid_argument);

    ASSERT_NO_THROW(new wrench::StorageServiceFileContentChunkMessage(file, 666, true));
    ASSERT_THROW(new wrench::StorageServiceFileContentChunkMessage(nullptr, 666, true), std::invalid_argument);
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

    ASSERT_NO_THROW(new wrench::CoordinateLookupAnswerMessage("requested_host", true, std::make_pair(1.0,1.0), 1.0, 666));
    ASSERT_THROW(new wrench::CoordinateLookupAnswerMessage("", true, std::make_pair(1.0, 1.0), 1.0, 666), std::invalid_argument);
}



TEST_F(MessageConstructorTest, BatchComputeServiceMessages) {

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

    ASSERT_NO_THROW(new wrench::BatchComputeServiceJobRequestMessage("mailbox", batch_job, 666));
    ASSERT_THROW(new wrench::BatchComputeServiceJobRequestMessage("mailbox", nullptr, 666), std::invalid_argument);
    ASSERT_THROW(new wrench::BatchComputeServiceJobRequestMessage("", batch_job, 666), std::invalid_argument);
    ASSERT_NO_THROW(new wrench::AlarmJobTimeOutMessage(batch_job, 666));
    ASSERT_THROW(new wrench::AlarmJobTimeOutMessage(nullptr, 666), std::invalid_argument);


//  ASSERT_NO_THROW(new wrench::AlarmNotifyBatschedMessage("job_id", 666));
}

