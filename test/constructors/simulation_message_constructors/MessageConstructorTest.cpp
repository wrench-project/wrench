/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */




#include <gtest/gtest.h>
#include <map>
#include <wrench/services/compute/batch/BatchComputeServiceMessage.h>

#include <wrench/execution_controller/ExecutionController.h>
#include <wrench/data_file/DataFile.h>
#include <wrench/workflow/Workflow.h>
#include "services/file_registry/FileRegistryMessage.h"
#include <wrench/services/compute/ComputeServiceMessage.h>
#include "services/storage/StorageServiceMessage.h"
#include "services/compute/cloud/CloudComputeServiceMessage.h"
#include "services/compute/virtualized_cluster/VirtualizedClusterComputeServiceMessage.h"
#include "services/network_proximity/NetworkProximityMessage.h"
#include <wrench/services/compute/batch/BatchJob.h>
#include <wrench/failure_causes/FatalFailure.h>
#include <wrench/managers/JobManager.h>
#include <wrench/services/storage/simple/SimpleStorageService.h>

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"

#define CUSTOM_THROW(x, y)  try { \
            x ; \
            throw std::runtime_error(std::string("Throwing at ") + __FILE__ + ":" + std::to_string(__LINE__)); \
        } catch (std::invalid_argument &ignore) {}

#define CUSTOM_NO_THROW(x)  try { \
            x ; \
        } catch (std::invalid_argument &e) { \
            throw std::runtime_error(std::string("Throwing at ") + __FILE__ + ":" + std::to_string(__LINE__) + ": " + e.what()); \
        }

class MessageConstructorTest : public ::testing::Test {

public:

    void do_MessageConstruction_test();

protected:

    ~MessageConstructorTest() {
    }

    MessageConstructorTest() {

        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();

        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "           </disk>  "
                          "       </host>  "
                          "       <host id=\"Host2\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk2\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "           </disk>  "
                          "       </host>  "
                          "       <link id=\"1\" bandwidth=\"100MBps\" latency=\"0us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
                          "   </zone> "
                          "</platform>";
        // Create a four-host 10-core platform file
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

    }
    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";

public:
    std::shared_ptr<wrench::Workflow> workflow;
    std::shared_ptr<wrench::StorageService> storage_service;
    std::shared_ptr<wrench::ComputeService> compute_service;
    std::shared_ptr<wrench::NetworkProximityService> network_proximity_service;
    std::shared_ptr<wrench::FailureCause> failure_cause;
    std::shared_ptr<wrench::BatchJob> batch_job;
    std::shared_ptr<wrench::NetworkProximityDaemon> network_proximity_daemon;


};

class MessageConstructorTestWMS : public wrench::ExecutionController {

public:
    MessageConstructorTestWMS(MessageConstructorTest *test,
                              std::shared_ptr<wrench::Workflow> workflow,
                              std::string hostname) :
            wrench::ExecutionController(hostname, "test"), test(test), workflow(workflow) {
    }


private:

    MessageConstructorTest *test;
    std::shared_ptr<wrench::Workflow> workflow;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();
        auto compound_job = job_manager->createCompoundJob("name");
        auto network_proximity_service = this->test->network_proximity_service;
        auto compute_service = this->test->compute_service;
        auto storage_service = this->test->storage_service;
        auto failure_cause = std::shared_ptr<wrench::FatalFailure>(new wrench::FatalFailure("msg"));
        auto network_proximity_daemon = std::shared_ptr<wrench::NetworkProximityDaemon>(
                new wrench::NetworkProximityDaemon(this->simulation, "Host1",
                                                   "mailbox",
                                                   10.0, 1.0,
                                                   1.0, 0, {}));
        auto batch_job = std::shared_ptr<wrench::BatchJob>(new wrench::BatchJob(compound_job, 1, 10, 1,
                                                                                1, "me", 10.0, 0.0));


        auto msg = new wrench::SimulationMessage("name", 666);
        if (msg->getName() != "name") {
            throw std::runtime_error(std::string("Throwing a ") + __FILE__ + ":" + std::to_string(__LINE__));
        }

        auto file = this->test->workflow->addFile("some_file", 100);
        auto location = wrench::FileLocation::LOCATION(this->test->storage_service);


        CUSTOM_THROW(new wrench::SimulationMessage("", 666), std:invalid_argument);
        CUSTOM_THROW(new wrench::SimulationMessage("name", -1), std::invalid_argument);


        CUSTOM_NO_THROW(new wrench::FileRegistryFileLookupRequestMessage("mailbox", file, 666));
        CUSTOM_THROW(new wrench::FileRegistryFileLookupRequestMessage("", file, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::FileRegistryFileLookupRequestMessage("mailbox", nullptr, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::FileRegistryFileLookupAnswerMessage(file, {}, 666));
        CUSTOM_THROW(new wrench::FileRegistryFileLookupAnswerMessage(nullptr, {}, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::FileRegistryRemoveEntryRequestMessage("mailbox", file, location, 666));
        CUSTOM_THROW(new wrench::FileRegistryRemoveEntryRequestMessage("", file, location, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::FileRegistryRemoveEntryRequestMessage("mailbox", nullptr, location, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::FileRegistryRemoveEntryRequestMessage("mailbox", file, nullptr, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::FileRegistryRemoveEntryAnswerMessage(true, 666));

        CUSTOM_NO_THROW(new wrench::FileRegistryAddEntryRequestMessage("mailbox", file, location, 666));
        CUSTOM_THROW(new wrench::FileRegistryAddEntryRequestMessage("", file, location, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::FileRegistryAddEntryRequestMessage("mailbox", nullptr, location, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::FileRegistryAddEntryRequestMessage("mailbox", file, nullptr, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::FileRegistryAddEntryAnswerMessage(666));

        CUSTOM_NO_THROW(new wrench::FileRegistryFileLookupByProximityRequestMessage("mailbox", file, "reference_host",
                                                                                    this->test->network_proximity_service, 666));
        CUSTOM_THROW(new wrench::FileRegistryFileLookupByProximityRequestMessage("", file, "reference_host",
                                                                                 network_proximity_service, 666),
                     std::invalid_argument);
        CUSTOM_THROW(new wrench::FileRegistryFileLookupByProximityRequestMessage("", nullptr, "reference_host",
                                                                                 network_proximity_service, 666),
                     std::invalid_argument);
        CUSTOM_THROW(
                new wrench::FileRegistryFileLookupByProximityRequestMessage("", file, "", network_proximity_service, 666),
                std::invalid_argument);
        CUSTOM_THROW(new wrench::FileRegistryFileLookupByProximityRequestMessage("", file, "reference_host", nullptr, 666),
                     std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::FileRegistryFileLookupByProximityAnswerMessage(file, "reference_host", {{}}, 666));
        CUSTOM_THROW(new wrench::FileRegistryFileLookupByProximityAnswerMessage(nullptr, "reference_host", {{}}, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::FileRegistryFileLookupByProximityAnswerMessage(file, "", {{}}, 666), std::invalid_argument);


        CUSTOM_NO_THROW(new wrench::ComputeServiceSubmitCompoundJobAnswerMessage(compound_job, compute_service, true, nullptr, 666));
        CUSTOM_NO_THROW(new wrench::ComputeServiceSubmitCompoundJobAnswerMessage(compound_job, compute_service, false, failure_cause, 666));
        CUSTOM_THROW(new wrench::ComputeServiceSubmitCompoundJobAnswerMessage(nullptr, compute_service, true, nullptr, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::ComputeServiceSubmitCompoundJobAnswerMessage(compound_job, nullptr, true, nullptr, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::ComputeServiceSubmitCompoundJobAnswerMessage(compound_job, compute_service, true, failure_cause, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::ComputeServiceSubmitCompoundJobAnswerMessage(compound_job, compute_service, false, nullptr, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::ComputeServiceCompoundJobDoneMessage(compound_job, compute_service, 666));
        CUSTOM_THROW(new wrench::ComputeServiceCompoundJobDoneMessage(nullptr, compute_service, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::ComputeServiceCompoundJobDoneMessage(compound_job, nullptr, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::ComputeServiceCompoundJobFailedMessage(compound_job, compute_service, 666));
        CUSTOM_THROW(new wrench::ComputeServiceCompoundJobFailedMessage(nullptr, compute_service, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::ComputeServiceCompoundJobFailedMessage(compound_job, nullptr, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::ComputeServiceTerminateCompoundJobRequestMessage("mailbox", compound_job, 666));
        CUSTOM_THROW(new wrench::ComputeServiceTerminateCompoundJobRequestMessage("", compound_job, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::ComputeServiceTerminateCompoundJobRequestMessage("mailbox", nullptr, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::ComputeServiceTerminateCompoundJobAnswerMessage(compound_job, compute_service, true, nullptr, 666));
        CUSTOM_NO_THROW(new wrench::ComputeServiceTerminateCompoundJobAnswerMessage(compound_job, compute_service, false, failure_cause, 666));
        CUSTOM_THROW(new wrench::ComputeServiceTerminateCompoundJobAnswerMessage(nullptr, compute_service, true, nullptr, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::ComputeServiceTerminateCompoundJobAnswerMessage(compound_job, nullptr, true, nullptr, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::ComputeServiceTerminateCompoundJobAnswerMessage(compound_job, compute_service, true, failure_cause, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::ComputeServiceTerminateCompoundJobAnswerMessage(compound_job, compute_service, false, nullptr, 666), std::invalid_argument);


        CUSTOM_NO_THROW(new wrench::ComputeServiceResourceInformationRequestMessage("mailbox", 666));
        CUSTOM_THROW(new wrench::ComputeServiceResourceInformationRequestMessage("", 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::ComputeServiceResourceInformationAnswerMessage({std::make_pair("something", std::map<std::string, double>({{"aa", 2.3}, {"bb", 4.5}}))}, 666));


        CUSTOM_NO_THROW(new wrench::CloudComputeServiceGetExecutionHostsRequestMessage("mailbox", 600));
        CUSTOM_THROW(new wrench::CloudComputeServiceGetExecutionHostsRequestMessage("", 666), std::invalid_argument);

        std::vector<std::string> arg;
        arg.emplace_back("aaa");
        CUSTOM_NO_THROW(new wrench::CloudComputeServiceGetExecutionHostsAnswerMessage(arg, 600));

        std::map<std::string, std::string> property_list;
        std::map<std::string, std::string> messagepayload_list;
        CUSTOM_NO_THROW(new wrench::CloudComputeServiceCreateVMRequestMessage("mailbox", 42, 10, "stuff", {}, {}, 666));
        CUSTOM_THROW(
                new wrench::CloudComputeServiceCreateVMRequestMessage("", 42, 0, "stuff", {}, {}, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::VirtualizedClusterComputeServiceMigrateVMRequestMessage("mailbox", "host", "host", 666));
        CUSTOM_THROW(new wrench::VirtualizedClusterComputeServiceMigrateVMRequestMessage("", "host", "host", 666),
                     std::invalid_argument);
        CUSTOM_THROW(new wrench::VirtualizedClusterComputeServiceMigrateVMRequestMessage("mailbox", "", "host", 666),
                     std::invalid_argument);
        CUSTOM_THROW(new wrench::VirtualizedClusterComputeServiceMigrateVMRequestMessage("mailbox", "host", "", 666),
                     std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::CloudComputeServiceShutdownVMRequestMessage("mailbox", "vm", false, wrench::ComputeService::TerminationCause::TERMINATION_NONE, 666));
        CUSTOM_THROW(new wrench::CloudComputeServiceShutdownVMRequestMessage("", "vm", false, wrench::ComputeService::TerminationCause::TERMINATION_NONE, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::CloudComputeServiceShutdownVMRequestMessage("mailbox", "", false, wrench::ComputeService::TerminationCause::TERMINATION_NONE, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::CloudComputeServiceShutdownVMAnswerMessage(true, nullptr, 666));

        CUSTOM_NO_THROW(new wrench::CloudComputeServiceStartVMRequestMessage("mailbox", "vm", "host", 666));
        CUSTOM_THROW(new wrench::CloudComputeServiceStartVMRequestMessage("", "vm", "host", 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::CloudComputeServiceStartVMRequestMessage("mailbox", "", "host", 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::CloudComputeServiceStartVMAnswerMessage(true, nullptr, nullptr, 666));

        CUSTOM_NO_THROW(new wrench::CloudComputeServiceSuspendVMRequestMessage("mailbox", "vm", 666));
        CUSTOM_THROW(new wrench::CloudComputeServiceSuspendVMRequestMessage("", "vm", 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::CloudComputeServiceSuspendVMRequestMessage("mailbox", "", 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::CloudComputeServiceSuspendVMAnswerMessage(true, nullptr, 666));

        CUSTOM_NO_THROW(new wrench::CloudComputeServiceResumeVMRequestMessage("mailbox", "vm", 666));
        CUSTOM_THROW(new wrench::CloudComputeServiceResumeVMRequestMessage("", "vm", 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::CloudComputeServiceResumeVMRequestMessage("mailbox", "", 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::CloudComputeServiceResumeVMAnswerMessage(true, nullptr, 666));

        CUSTOM_NO_THROW(new wrench::CloudComputeServiceDestroyVMRequestMessage("mailbox", "vm", 666));
        CUSTOM_THROW(new wrench::CloudComputeServiceDestroyVMRequestMessage("", "vm", 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::CloudComputeServiceDestroyVMRequestMessage("mailbox", "", 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::CloudComputeServiceDestroyVMAnswerMessage(true, nullptr, 666));

        CUSTOM_NO_THROW(new wrench::StorageServiceFreeSpaceRequestMessage("mailbox", 666));
        CUSTOM_THROW(new wrench::StorageServiceFreeSpaceRequestMessage("", 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::StorageServiceFreeSpaceAnswerMessage({{"/", 1000}, {"/foo/", 2000}}, 666));
        CUSTOM_THROW(new wrench::StorageServiceFreeSpaceAnswerMessage({{"/", -10.0}}, 666), std::invalid_argument);

        std::string root_dir = "/";
        CUSTOM_NO_THROW(new wrench::StorageServiceFileLookupRequestMessage("mailbox", file, location, 666));
        CUSTOM_THROW(new wrench::StorageServiceFileLookupRequestMessage("", file, location, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileLookupRequestMessage("mailbox", nullptr, location, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileLookupRequestMessage("mailbox", file, nullptr, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::StorageServiceFileLookupAnswerMessage(file, true, 666));
        CUSTOM_THROW(new wrench::StorageServiceFileLookupAnswerMessage(nullptr, true, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::StorageServiceFileDeleteRequestMessage("mailbox", file, location, 666));
        CUSTOM_THROW(new wrench::StorageServiceFileDeleteRequestMessage("", file, location, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileDeleteRequestMessage("mailbox", nullptr, location, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileDeleteRequestMessage("mailbox", file, nullptr, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::StorageServiceFileDeleteAnswerMessage(file, storage_service, true, nullptr, 666));
        CUSTOM_NO_THROW(new wrench::StorageServiceFileDeleteAnswerMessage(file, storage_service, false, failure_cause, 666));
        CUSTOM_THROW(new wrench::StorageServiceFileDeleteAnswerMessage(nullptr, storage_service, true, failure_cause, 666),
                     std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileDeleteAnswerMessage(file, nullptr, true, failure_cause, 666),
                     std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileDeleteAnswerMessage(file, storage_service, false, nullptr, 666),
                     std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileDeleteAnswerMessage(file, storage_service, true, failure_cause, 666),
                     std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::StorageServiceFileCopyRequestMessage("mailbox", file, location, location, nullptr, 666));
        CUSTOM_THROW(new wrench::StorageServiceFileCopyRequestMessage("", file, location, location, nullptr, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileCopyRequestMessage("mailbox", nullptr, location, location, nullptr, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileCopyRequestMessage("mailbox", file, nullptr, location, nullptr, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileCopyRequestMessage("mailbox", file, location, nullptr, nullptr, 666), std::invalid_argument);

        CUSTOM_THROW(new wrench::StorageServiceFileCopyAnswerMessage(nullptr, location, location, nullptr, false, true, nullptr, 666), std::invalid_argument);
        CUSTOM_NO_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, location, location, nullptr, false, true, nullptr, 666));
        CUSTOM_NO_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, location, location, nullptr, false, false, failure_cause, 666));
        CUSTOM_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, nullptr, nullptr, nullptr, false, true, nullptr, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, location, nullptr, nullptr, false, true, nullptr, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, location, nullptr, nullptr, true, true, nullptr, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, location, nullptr, nullptr, false, false, nullptr, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileCopyAnswerMessage(file, location, nullptr, nullptr, false, true, failure_cause, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::StorageServiceFileWriteRequestMessage("mailbox", file, location, 10, 666));
        CUSTOM_THROW(new wrench::StorageServiceFileWriteRequestMessage("", nullptr, location, 10, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileWriteRequestMessage("mailbox", file, nullptr, 10, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::StorageServiceFileWriteAnswerMessage(file, location, true, nullptr, "mailbox", 666));
        CUSTOM_NO_THROW(new wrench::StorageServiceFileWriteAnswerMessage(file, location, false, failure_cause, "", 666));
        CUSTOM_THROW(new wrench::StorageServiceFileWriteAnswerMessage(nullptr, location, true, nullptr, "mailbox", 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileWriteAnswerMessage(file, nullptr, true, nullptr, "mailbox", 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileWriteAnswerMessage(file, location, true, nullptr, "", 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileWriteAnswerMessage(file, location, true, failure_cause, "mailbox", 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileWriteAnswerMessage(file, location, false, nullptr, "mailbox", 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::StorageServiceFileReadRequestMessage("mailbox", "mailbox", file, location, 10, 10, 666));
        CUSTOM_THROW(new wrench::StorageServiceFileReadRequestMessage("", "mailbox", file, location, 10, 10, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileReadRequestMessage("mailbox", "", file, location, 10, 10, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileReadRequestMessage("mailbox", "mailbox", nullptr, location, 10, 10, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileReadRequestMessage("mailbox", "mailbox", file, nullptr, 10, 10, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileReadRequestMessage("mailbox", "mailbox", file, location, -1.0, 10, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::StorageServiceFileReadAnswerMessage(file, location, true, nullptr, 666));
        CUSTOM_NO_THROW(new wrench::StorageServiceFileReadAnswerMessage(file, location, false, failure_cause, 666));
        CUSTOM_THROW(new wrench::StorageServiceFileReadAnswerMessage(nullptr, location, true, nullptr, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileReadAnswerMessage(file, nullptr, true, nullptr, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileReadAnswerMessage(file, location, true, failure_cause, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileReadAnswerMessage(file, location, false, nullptr, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::StorageServiceFileContentChunkMessage(file, 666, true));
        CUSTOM_THROW(new wrench::StorageServiceFileContentChunkMessage(nullptr, 666, true), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::NetworkProximityLookupRequestMessage("mailbox", std::make_pair("a","b"), 666));
        CUSTOM_THROW(new wrench::NetworkProximityLookupRequestMessage("", std::make_pair("a","b"), 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::NetworkProximityLookupRequestMessage("mailbox", std::make_pair("","b"), 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::NetworkProximityLookupRequestMessage("mailbox", std::make_pair("a",""), 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::NetworkProximityLookupAnswerMessage(std::make_pair("a","b"), 1.0, 1.0, 666));
        CUSTOM_THROW(new wrench::NetworkProximityLookupAnswerMessage(std::make_pair("","b"), 1.0, 1.0, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::NetworkProximityLookupAnswerMessage(std::make_pair("a",""), 1.0, 1.0, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::NetworkProximityComputeAnswerMessage(std::make_pair("a","b"), 1.0, 666));
        CUSTOM_THROW(new wrench::NetworkProximityComputeAnswerMessage(std::make_pair("","b"), 1.0, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::NetworkProximityComputeAnswerMessage(std::make_pair("a",""), 1.0, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::NextContactDaemonRequestMessage(network_proximity_daemon, 666));
        CUSTOM_THROW(new wrench::NextContactDaemonRequestMessage(nullptr, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::CoordinateLookupRequestMessage("mailbox", "requested_host", 666));
        CUSTOM_THROW(new wrench::CoordinateLookupRequestMessage("", "requested_host", 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::CoordinateLookupRequestMessage("mailbox", "", 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::CoordinateLookupAnswerMessage("requested_host", true, std::make_pair(1.0,1.0), 1.0, 666));
        CUSTOM_THROW(new wrench::CoordinateLookupAnswerMessage("", true, std::make_pair(1.0, 1.0), 1.0, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::BatchExecuteJobFromBatSchedMessage("mailbox", "string", 666));
        CUSTOM_THROW(new wrench::BatchExecuteJobFromBatSchedMessage("", "string", 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::BatchExecuteJobFromBatSchedMessage("mailbox", "", 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::BatchQueryAnswerMessage(1.0, 666));

        CUSTOM_NO_THROW(new wrench::BatchComputeServiceJobRequestMessage("mailbox", batch_job, 666));
        CUSTOM_THROW(new wrench::BatchComputeServiceJobRequestMessage("mailbox", nullptr, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::BatchComputeServiceJobRequestMessage("", batch_job, 666), std::invalid_argument);
        CUSTOM_NO_THROW(new wrench::AlarmJobTimeOutMessage(batch_job, 666));
        CUSTOM_THROW(new wrench::AlarmJobTimeOutMessage(nullptr, 666), std::invalid_argument);

        return 0;
    }
};


TEST_F(MessageConstructorTest, AllMessages) {
    DO_TEST_WITH_FORK(do_MessageConstruction_test);
}

void MessageConstructorTest::do_MessageConstruction_test() {


    // Create and initialize a simulation
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
//    argv[1] = strdup("--wrench-full-log");

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    simulation->init(&argc, argv);

    // Setting up the platform
    CUSTOM_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create services
    this->storage_service = simulation->add(new wrench::SimpleStorageService("Host1", {"/"}));
    this->network_proximity_service = simulation->add(new wrench::NetworkProximityService("Host1", {"Host1", "Host2"}));
    this->compute_service = simulation->add(new wrench::BareMetalComputeService("Host1", {"Host1"}, ""));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    CUSTOM_NO_THROW(wms = simulation->add(new MessageConstructorTestWMS(this, workflow, hostname)));

    simulation->launch();

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);


}

