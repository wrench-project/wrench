/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the xterms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <wrench-dev.h>
#include <wrench/services/helpers/ServiceTerminationDetectorMessage.h>

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"
#include "test_util/HostSwitcher.h"
#include "wrench/services/helpers/ServiceTerminationDetector.h"
#include "test_util/SleeperVictim.h"
#include "test_util/HostRandomRepeatSwitcher.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(cloud_compute_service_simulated_failures_test, "Log category for CloudServiceSimulatedFailuresTests");


class CloudServiceSimulatedFailuresTest : public ::testing::Test {

public:
    wrench::Workflow *workflow;
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr;

    wrench::WorkflowFile *input_file;
    wrench::WorkflowFile *output_file;
    wrench::WorkflowTask *task;
    wrench::StorageService *storage_service = nullptr;
    wrench::ComputeService *compute_service = nullptr;

    void do_CloudServiceFailureOfAVMWithRunningJob_test();

protected:

    CloudServiceSimulatedFailuresTest() {
        // Create the simplest workflow
        workflow_unique_ptr = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
        workflow = workflow_unique_ptr.get();

        // Create two files
        input_file = workflow->addFile("input_file", 10000.0);
        output_file = workflow->addFile("output_file", 20000.0);

        // Create one task
        task = workflow->addTask("task", 3600, 1, 1, 1.0, 0);
        task->addInputFile(input_file);
        task->addOutputFile(output_file);

        // Create a platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"FailedHost1\" speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"FailedHost2\" speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"StableHost\" speed=\"1f\" core=\"1\"/> "
                          "       <link id=\"link1\" bandwidth=\"100kBps\" latency=\"0\"/>"
                          "       <route src=\"FailedHost1\" dst=\"StableHost\">"
                          "           <link_ctn id=\"link1\"/>"
                          "       </route>"
                          "       <route src=\"FailedHost2\" dst=\"StableHost\">"
                          "           <link_ctn id=\"link1\"/>"
                          "       </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};






/**********************************************************************/
/**                    FAILURE WITH RUNNING JOB                      **/
/**********************************************************************/

class CloudServiceFailureOfAVMTestWMS : public wrench::WMS {

public:
    CloudServiceFailureOfAVMTestWMS(CloudServiceSimulatedFailuresTest *test,
                                    std::string &hostname, wrench::ComputeService *cs, wrench::StorageService *ss) :
            wrench::WMS(nullptr, nullptr, {cs}, {ss}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    CloudServiceSimulatedFailuresTest *test;

    int main() override {

        // Starting a FailedHost1 murderer!!
        auto murderer = std::shared_ptr<wrench::HostSwitcher>(new wrench::HostSwitcher("StableHost", 100, "FailedHost1", wrench::HostSwitcher::Action::TURN_OFF));
        murderer->simulation = this->simulation;
        murderer->start(murderer, true, false); // Daemonized, no auto-restart

        wrench::Simulation::sleep(10);

        // Create a VM on the Cloud Service
        auto cloud_service = (wrench::CloudService *)this->test->compute_service;
        auto vm_name = cloud_service->createVM(1, this->test->task->getMemoryRequirement());
        auto vm_cs = cloud_service->startVM(vm_name);

        // Create a job manager
        std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

        // Create a standard job
        auto job = job_manager->createStandardJob(this->test->task, {{this->test->input_file, this->test->storage_service},
                                                                     {this->test->output_file, this->test->storage_service}});

        // Submit the standard job to the compute service, making it sure it runs on FailedHost1
        job_manager->submitJob(job, vm_cs.get());

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event = this->getWorkflow()->waitForNextExecutionEvent();
        if (event->type != wrench::WorkflowExecutionEvent::STANDARD_JOB_FAILURE) {
            throw std::runtime_error("Unexpected workflow execution event!");
        }
        auto failure_cause = dynamic_cast<wrench::StandardJobFailedEvent *>(event.get())->failure_cause;
        if (failure_cause->getCauseType() != wrench::FailureCause::JOB_KILLED) {
            throw std::runtime_error("Invalid failure cause type: should be JOB_KILLED but was " + std::to_string(failure_cause->getCauseType()));
        }
        auto real_failure = dynamic_cast<wrench::JobKilled *>(failure_cause.get());
        if (real_failure->getJob() != job) {
            throw std::runtime_error("Failure cause does not point to the correct job");
        }
        if (real_failure->getComputeService() != vm_cs.get()) {
            throw std::runtime_error("Failure cause does not point to the correct compute service");
        }

        // Check that the VM is down
        if (not cloud_service->isVMDown(vm_name)) {
            throw std::runtime_error("The VM should be down!");
        }

        // Restart the VM (which shouldn't work because the Cloud has no more resources right now)
        try {
            cloud_service->startVM(vm_name);
            throw std::runtime_error("Should not be able to restart VM since the CloudService no longer has resources!");
        } catch (wrench::WorkflowExecutionException &e) {
            // expected
        }

        return 0;
    }
};

TEST_F(CloudServiceSimulatedFailuresTest, FailureOfAVMWithRunningJob) {
    DO_TEST_WITH_FORK(do_CloudServiceFailureOfAVMWithRunningJob_test);
}

void CloudServiceSimulatedFailuresTest::do_CloudServiceFailureOfAVMWithRunningJob_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("failure_test");


    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string stable_host = "StableHost";
    std::vector<std::string> compute_hosts;
    compute_hosts.push_back("FailedHost1");

    // Create a Compute Service that has access to two hosts
    compute_service = simulation->add(
            new wrench::CloudService(stable_host,
                                     compute_hosts,
                                     100.0,
                                     {}));

    // Create a Storage Service
    storage_service = simulation->add(new wrench::SimpleStorageService(stable_host, 10000000000000.0));

    // Create a WMS
    wrench::WMS *wms = nullptr;
    wms = simulation->add(new CloudServiceFailureOfAVMTestWMS(this, stable_host, compute_service, storage_service));

    wms->addWorkflow(workflow);

    // Staging the input_file on the storage service
    // Create a File Registry Service
    simulation->add(new wrench::FileRegistryService(stable_host));
    simulation->stageFiles({{input_file->getID(), input_file}}, storage_service);

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}



