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
#include "wrench/services/file_registry/FileRegistryMessage.h"
#include <wrench/services/compute/ComputeServiceMessage.h>
#include "wrench/services/storage/StorageServiceMessage.h"
#include "wrench/services/compute/cloud/CloudComputeServiceMessage.h"
#include "wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeServiceMessage.h"
#include "wrench/services/network_proximity/NetworkProximityMessage.h"
#include <wrench/services/compute/batch/BatchJob.h>
#include <wrench/failure_causes/FatalFailure.h>
#include "wrench/managers/job_manager/JobManager.h"
#include <wrench/services/storage/simple/SimpleStorageService.h>

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"

#ifdef WRENCH_INTERNAL_EXCEPTIONS
#define CUSTOM_THROW(x, y)                                                                                 \
    try {                                                                                                  \
        x;                                                                                                 \
        throw std::runtime_error(std::string("Throwing at ") + __FILE__ + ":" + std::to_string(__LINE__)); \
    } catch (std::invalid_argument & ignore) {}
#else
#define CUSTOM_THROW(x, y) \
    {}
#endif

#define CUSTOM_NO_THROW(x)                                                                                                   \
    try {                                                                                                                    \
        x;                                                                                                                   \
    } catch (std::invalid_argument & e) {                                                                                    \
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
    std::shared_ptr<wrench::NetworkProximitySenderDaemon> network_proximity_daemon;
};

class MessageConstructorTestWMS : public wrench::ExecutionController {

public:
    MessageConstructorTestWMS(MessageConstructorTest *test,
                              std::shared_ptr<wrench::Workflow> workflow,
                              std::string hostname) : wrench::ExecutionController(hostname, "test"), test(test), workflow(workflow) {
    }


private:
    MessageConstructorTest *test;
    std::shared_ptr<wrench::Workflow> workflow;

    int main() override {

        // Create a job manager
        auto job_manager = this->createJobManager();
        auto compound_job = job_manager->createCompoundJob("name");
        auto network_proximity_service = this->test->network_proximity_service;
        auto compute_service = this->test->compute_service;
        auto storage_service = this->test->storage_service;
        auto failure_cause = std::shared_ptr<wrench::FatalFailure>(new wrench::FatalFailure("msg"));
        auto commport = wrench::S4U_CommPort::getTemporaryCommPort();

        auto network_proximity_daemon = std::shared_ptr<wrench::NetworkProximitySenderDaemon>(
                new wrench::NetworkProximitySenderDaemon(this->getSimulation(), "Host1",
                                                         commport,
                                                         10.0, 1.0,
                                                         1.0, 0, {}));
        auto batch_job = std::make_shared<wrench::BatchJob>(compound_job, 1, 10, 1,
                                                            1, "me", 10.0, 0.0);


        auto msg = new wrench::SimulationMessage(666);
        if (msg->getName().empty()) {
            throw std::runtime_error(std::string("Throwing a ") + __FILE__ + ":" + std::to_string(__LINE__));
        }

        auto file = wrench::Simulation::addFile("some_file", 100);
        auto location = wrench::FileLocation::LOCATION(this->test->storage_service, file);

        //CUSTOM_THROW(new wrench::SimulationMessage("", 666), std:invalid_argument);//names are no longer a thing
        CUSTOM_THROW(new wrench::SimulationMessage(-1), std::invalid_argument);


        CUSTOM_NO_THROW(new wrench::FileRegistryFileLookupRequestMessage(commport, file, 666));
        CUSTOM_THROW(new wrench::FileRegistryFileLookupRequestMessage(nullptr, file, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::FileRegistryFileLookupRequestMessage(commport, nullptr, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::FileRegistryFileLookupAnswerMessage({location}, 666));
        //        CUSTOM_THROW(new wrench::FileRegistryFileLookupAnswerMessage({}, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::FileRegistryRemoveEntryRequestMessage(commport, location, 666));
        CUSTOM_THROW(new wrench::FileRegistryRemoveEntryRequestMessage(nullptr, location, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::FileRegistryRemoveEntryRequestMessage(commport, nullptr, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::FileRegistryRemoveEntryAnswerMessage(true, 666));

        CUSTOM_NO_THROW(new wrench::FileRegistryAddEntryRequestMessage(commport, location, 666));
        CUSTOM_THROW(new wrench::FileRegistryAddEntryRequestMessage(nullptr, location, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::FileRegistryAddEntryRequestMessage(commport, nullptr, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::FileRegistryAddEntryAnswerMessage(666));

        CUSTOM_NO_THROW(new wrench::FileRegistryFileLookupByProximityRequestMessage(commport, file, "reference_host",
                                                                                    this->test->network_proximity_service, 666));
        CUSTOM_THROW(new wrench::FileRegistryFileLookupByProximityRequestMessage(nullptr, file, "reference_host",
                                                                                 network_proximity_service, 666),
                     std::invalid_argument);
        CUSTOM_THROW(new wrench::FileRegistryFileLookupByProximityRequestMessage(nullptr, nullptr, "reference_host",
                                                                                 network_proximity_service, 666),
                     std::invalid_argument);
        CUSTOM_THROW(
                new wrench::FileRegistryFileLookupByProximityRequestMessage(nullptr, file, "", network_proximity_service, 666),
                std::invalid_argument);
        CUSTOM_THROW(new wrench::FileRegistryFileLookupByProximityRequestMessage(nullptr, file, "reference_host", nullptr, 666),
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

        CUSTOM_NO_THROW(new wrench::ComputeServiceTerminateCompoundJobRequestMessage(commport, compound_job, 666));
        CUSTOM_THROW(new wrench::ComputeServiceTerminateCompoundJobRequestMessage(nullptr, compound_job, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::ComputeServiceTerminateCompoundJobRequestMessage(commport, nullptr, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::ComputeServiceTerminateCompoundJobAnswerMessage(compound_job, compute_service, true, nullptr, 666));
        CUSTOM_NO_THROW(new wrench::ComputeServiceTerminateCompoundJobAnswerMessage(compound_job, compute_service, false, failure_cause, 666));
        CUSTOM_THROW(new wrench::ComputeServiceTerminateCompoundJobAnswerMessage(nullptr, compute_service, true, nullptr, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::ComputeServiceTerminateCompoundJobAnswerMessage(compound_job, nullptr, true, nullptr, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::ComputeServiceTerminateCompoundJobAnswerMessage(compound_job, compute_service, true, failure_cause, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::ComputeServiceTerminateCompoundJobAnswerMessage(compound_job, compute_service, false, nullptr, 666), std::invalid_argument);


        CUSTOM_NO_THROW(new wrench::ComputeServiceResourceInformationRequestMessage(commport, "stuff", 666));
        CUSTOM_THROW(new wrench::ComputeServiceResourceInformationRequestMessage(nullptr, "stuff", 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::ComputeServiceResourceInformationRequestMessage(commport, "", 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::ComputeServiceResourceInformationAnswerMessage({std::map<std::string, double>({{"aa", 2.3}, {"bb", 4.5}})}, 666));


        CUSTOM_NO_THROW(new wrench::CloudComputeServiceGetExecutionHostsRequestMessage(commport, 600));
        CUSTOM_THROW(new wrench::CloudComputeServiceGetExecutionHostsRequestMessage(nullptr, 666), std::invalid_argument);

        std::vector<std::string> arg;
        arg.emplace_back("aaa");
        CUSTOM_NO_THROW(new wrench::CloudComputeServiceGetExecutionHostsAnswerMessage(arg, 600));

        std::map<std::string, std::string> property_list;
        std::map<std::string, std::string> messagepayload_list;
        CUSTOM_NO_THROW(new wrench::CloudComputeServiceCreateVMRequestMessage(commport, 42, 10, "stuff", {}, {}, 666));
        CUSTOM_THROW(
                new wrench::CloudComputeServiceCreateVMRequestMessage(nullptr, 42, 0, "stuff", {}, {}, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::VirtualizedClusterComputeServiceMigrateVMRequestMessage(commport, "host", "host", 666));
        CUSTOM_THROW(new wrench::VirtualizedClusterComputeServiceMigrateVMRequestMessage(nullptr, "host", "host", 666),
                     std::invalid_argument);
        CUSTOM_THROW(new wrench::VirtualizedClusterComputeServiceMigrateVMRequestMessage(commport, "", "host", 666),
                     std::invalid_argument);
        CUSTOM_THROW(new wrench::VirtualizedClusterComputeServiceMigrateVMRequestMessage(commport, "host", "", 666),
                     std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::CloudComputeServiceShutdownVMRequestMessage(commport, "vm", false, wrench::ComputeService::TerminationCause::TERMINATION_NONE, 666));
        CUSTOM_THROW(new wrench::CloudComputeServiceShutdownVMRequestMessage(nullptr, "vm", false, wrench::ComputeService::TerminationCause::TERMINATION_NONE, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::CloudComputeServiceShutdownVMRequestMessage(commport, "", false, wrench::ComputeService::TerminationCause::TERMINATION_NONE, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::CloudComputeServiceShutdownVMAnswerMessage(true, nullptr, 666));

        CUSTOM_NO_THROW(new wrench::CloudComputeServiceStartVMRequestMessage(commport, "vm", 666));
        CUSTOM_THROW(new wrench::CloudComputeServiceStartVMRequestMessage(nullptr, "vm", 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::CloudComputeServiceStartVMRequestMessage(commport, "", 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::CloudComputeServiceStartVMAnswerMessage(true, nullptr, nullptr, 666));

        CUSTOM_NO_THROW(new wrench::CloudComputeServiceSuspendVMRequestMessage(commport, "vm", 666));
        CUSTOM_THROW(new wrench::CloudComputeServiceSuspendVMRequestMessage(nullptr, "vm", 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::CloudComputeServiceSuspendVMRequestMessage(commport, "", 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::CloudComputeServiceSuspendVMAnswerMessage(true, nullptr, 666));

        CUSTOM_NO_THROW(new wrench::CloudComputeServiceResumeVMRequestMessage(commport, "vm", 666));
        CUSTOM_THROW(new wrench::CloudComputeServiceResumeVMRequestMessage(nullptr, "vm", 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::CloudComputeServiceResumeVMRequestMessage(commport, "", 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::CloudComputeServiceResumeVMAnswerMessage(true, nullptr, 666));

        CUSTOM_NO_THROW(new wrench::CloudComputeServiceDestroyVMRequestMessage(commport, "vm", 666));
        CUSTOM_THROW(new wrench::CloudComputeServiceDestroyVMRequestMessage(nullptr, "vm", 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::CloudComputeServiceDestroyVMRequestMessage(commport, "", 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::CloudComputeServiceDestroyVMAnswerMessage(true, nullptr, 666));

        CUSTOM_NO_THROW(new wrench::StorageServiceFreeSpaceRequestMessage(commport, "/", 666));
        CUSTOM_NO_THROW(new wrench::StorageServiceFreeSpaceRequestMessage(commport, "", 666));
        CUSTOM_THROW(new wrench::StorageServiceFreeSpaceRequestMessage(nullptr, "/", 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::StorageServiceFreeSpaceAnswerMessage(1000, 666));
        CUSTOM_THROW(new wrench::StorageServiceFreeSpaceAnswerMessage(-10, 666), std::invalid_argument);

        std::string root_dir = "/";
        CUSTOM_NO_THROW(new wrench::StorageServiceFileLookupRequestMessage(commport, location, 666));
        CUSTOM_THROW(new wrench::StorageServiceFileLookupRequestMessage(nullptr, location, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileLookupRequestMessage(commport, nullptr, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::StorageServiceFileLookupAnswerMessage(file, true, 666));
        CUSTOM_THROW(new wrench::StorageServiceFileLookupAnswerMessage(nullptr, true, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::StorageServiceFileDeleteRequestMessage(commport, location, 666));
        CUSTOM_THROW(new wrench::StorageServiceFileDeleteRequestMessage(nullptr, location, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileDeleteRequestMessage(commport, nullptr, 666), std::invalid_argument);

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

        CUSTOM_NO_THROW(new wrench::StorageServiceFileCopyRequestMessage(commport, location, location, 666));
        CUSTOM_THROW(new wrench::StorageServiceFileCopyRequestMessage(nullptr, location, location, nullptr, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileCopyRequestMessage(commport, nullptr, location, nullptr, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileCopyRequestMessage(commport, location, nullptr, nullptr, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::StorageServiceFileCopyAnswerMessage(location, location, true, nullptr, 666));
        CUSTOM_NO_THROW(new wrench::StorageServiceFileCopyAnswerMessage(location, location, false, failure_cause, 666));
        CUSTOM_THROW(new wrench::StorageServiceFileCopyAnswerMessage(nullptr, nullptr, nullptr, false, true, nullptr, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileCopyAnswerMessage(location, nullptr, nullptr, false, true, nullptr, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileCopyAnswerMessage(location, nullptr, nullptr, true, true, nullptr, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileCopyAnswerMessage(location, nullptr, nullptr, false, false, nullptr, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileCopyAnswerMessage(location, nullptr, nullptr, false, true, failure_cause, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::StorageServiceFileWriteRequestMessage(commport, (simgrid::s4u::Host *) 666, location, 1024, 666));
        CUSTOM_THROW(new wrench::StorageServiceFileWriteRequestMessage(nullptr, (simgrid::s4u::Host *) 666, location, 10, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileWriteRequestMessage(commport, nullptr, location, 10, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileWriteRequestMessage(commport, (simgrid::s4u::Host *) 666, nullptr, 10, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::StorageServiceFileWriteAnswerMessage(location, true, nullptr, {{commport, 0.0}}, 10, 666));
        CUSTOM_NO_THROW(new wrench::StorageServiceFileWriteAnswerMessage(location, false, failure_cause, {{nullptr, 0.0}}, 10, 666));
        CUSTOM_THROW(new wrench::StorageServiceFileWriteAnswerMessage(nullptr, true, nullptr, commport, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileWriteAnswerMessage(location, true, nullptr, nullptr, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileWriteAnswerMessage(location, true, failure_cause, commport, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileWriteAnswerMessage(location, false, nullptr, commport, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::StorageServiceFileReadRequestMessage(commport, (simgrid::s4u::Host *) 666, location, 10, 666));
        CUSTOM_THROW(new wrench::StorageServiceFileReadRequestMessage(nullptr, (simgrid::s4u::Host *) 666, location, 10, 10, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileReadRequestMessage(commport, (simgrid::s4u::Host *) 666, location, 10, 10, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileReadRequestMessage(commport, (simgrid::s4u::Host *) 666, nullptr, 10, 10, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileReadRequestMessage(commport, (simgrid::s4u::Host *) 666, location, -1.0, 10, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::StorageServiceFileReadAnswerMessage(location, true, nullptr, commport, 10, 1, 666));
        CUSTOM_NO_THROW(new wrench::StorageServiceFileReadAnswerMessage(location, false, failure_cause, commport, 0, 1, 666));
        CUSTOM_THROW(new wrench::StorageServiceFileReadAnswerMessage(nullptr, true, nullptr, commport, 10, 1, 1, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileReadAnswerMessage(location, true, failure_cause, commport, 10, 1, 1, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::StorageServiceFileReadAnswerMessage(location, false, nullptr, commport, 10, 1, 1, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::StorageServiceFileContentChunkMessage(file, 666, true));
        CUSTOM_THROW(new wrench::StorageServiceFileContentChunkMessage(nullptr, 666, true), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::NetworkProximityLookupRequestMessage(commport, std::make_pair("a", "b"), 666));
        CUSTOM_THROW(new wrench::NetworkProximityLookupRequestMessage(nullptr, std::make_pair("a", "b"), 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::NetworkProximityLookupRequestMessage(commport, std::make_pair("", "b"), 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::NetworkProximityLookupRequestMessage(commport, std::make_pair("a", ""), 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::NetworkProximityLookupAnswerMessage(std::make_pair("a", "b"), 1.0, 1.0, 666));
        CUSTOM_THROW(new wrench::NetworkProximityLookupAnswerMessage(std::make_pair("", "b"), 1.0, 1.0, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::NetworkProximityLookupAnswerMessage(std::make_pair("a", ""), 1.0, 1.0, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::NetworkProximityComputeAnswerMessage(std::make_pair("a", "b"), 1.0, 666));
        CUSTOM_THROW(new wrench::NetworkProximityComputeAnswerMessage(std::make_pair("", "b"), 1.0, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::NetworkProximityComputeAnswerMessage(std::make_pair("a", ""), 1.0, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::NextContactDaemonRequestMessage(network_proximity_daemon, 666));
        CUSTOM_THROW(new wrench::NextContactDaemonRequestMessage(nullptr, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::CoordinateLookupRequestMessage(commport, "requested_host", 666));
        CUSTOM_THROW(new wrench::CoordinateLookupRequestMessage(nullptr, "requested_host", 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::CoordinateLookupRequestMessage(commport, "", 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::CoordinateLookupAnswerMessage("requested_host", true, std::make_pair(1.0, 1.0), 1.0, 666));
        CUSTOM_THROW(new wrench::CoordinateLookupAnswerMessage("", true, std::make_pair(1.0, 1.0), 1.0, 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::BatchExecuteJobFromBatSchedMessage(commport, "string", 666));
        CUSTOM_THROW(new wrench::BatchExecuteJobFromBatSchedMessage(nullptr, "string", 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::BatchExecuteJobFromBatSchedMessage(commport, "", 666), std::invalid_argument);

        CUSTOM_NO_THROW(new wrench::BatchQueryAnswerMessage(1.0, 666));

        CUSTOM_NO_THROW(new wrench::BatchComputeServiceJobRequestMessage(commport, batch_job, 666));
        CUSTOM_THROW(new wrench::BatchComputeServiceJobRequestMessage(commport, nullptr, 666), std::invalid_argument);
        CUSTOM_THROW(new wrench::BatchComputeServiceJobRequestMessage(nullptr, batch_job, 666), std::invalid_argument);
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
    //        argv[1] = strdup("--wrench-full-log");

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    simulation->init(&argc, argv);

    // Setting up the platform
    CUSTOM_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create services
    this->storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService("Host1", {"/"}, {}, {}));
    this->network_proximity_service = simulation->add(new wrench::NetworkProximityService("Host1", {"Host1", "Host2"}));
    this->compute_service = simulation->add(new wrench::BareMetalComputeService("Host1", {"Host1"}, ""));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    CUSTOM_NO_THROW(wms = simulation->add(new MessageConstructorTestWMS(this, workflow, hostname)));

    simulation->launch();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
