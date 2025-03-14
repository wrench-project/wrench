
/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the xterms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <wrench-dev.h>
#include <wrench/services/helper_services/service_termination_detector/ServiceTerminationDetectorMessage.h>

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"
#include "../failure_test_util/ResourceSwitcher.h"
#include <wrench/services/helper_services/service_termination_detector/ServiceTerminationDetector.h>
#include "../failure_test_util/SleeperVictim.h"
#include "../failure_test_util/ResourceRandomRepeatSwitcher.h"

WRENCH_LOG_CATEGORY(cloud_compute_service_host_failures_test, "Log category for CloudServiceHostFailuresTest");


class CloudServiceHostFailuresTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::Workflow> workflow;

    std::shared_ptr<wrench::DataFile> input_file;
    std::shared_ptr<wrench::DataFile> output_file;
    std::shared_ptr<wrench::WorkflowTask> task;
    std::shared_ptr<wrench::StorageService> storage_service = nullptr;
    std::shared_ptr<wrench::CloudComputeService> compute_service = nullptr;

    void do_CloudServiceFailureOfAVMWithRunningJob_test();
    void do_CloudServiceFailureOfAVMWithRunningJobFollowedByRestart_test();
    void do_CloudServiceRandomFailures_test();

protected:
    ~CloudServiceHostFailuresTest() override {
        workflow->clear();
        wrench::Simulation::removeAllFiles();
    }

    CloudServiceHostFailuresTest() {
        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();


        // Create two files
        input_file = wrench::Simulation::addFile("input_file", 10000);
        output_file = wrench::Simulation::addFile("output_file", 20000);

        // Create one task1
        task = workflow->addTask("task1", 3600, 1, 1, 0);
        task->addInputFile(input_file);
        task->addOutputFile(output_file);

        // Create a platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"FailedHost1\" speed=\"1f\" core=\"1\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"101B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>  "
                          "       <host id=\"FailedHost2\" speed=\"1f\" core=\"1\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"101B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>  "
                          "       <host id=\"StableHost\" speed=\"1f\" core=\"1\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"101B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>  "
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

class CloudServiceFailureOfAVMTestWMS : public wrench::ExecutionController {

public:
    CloudServiceFailureOfAVMTestWMS(CloudServiceHostFailuresTest *test,
                                    std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    CloudServiceHostFailuresTest *test;

    int main() override {

        // Starting a FailedHost1 murderer!!
        auto murderer = std::make_shared<wrench::ResourceSwitcher>("StableHost", 100, "FailedHost1",
                                                                   wrench::ResourceSwitcher::Action::TURN_OFF,
                                                                   wrench::ResourceSwitcher::ResourceType::HOST);
        murderer->setSimulation(this->getSimulation());
        murderer->start(murderer, true, false);// Daemonized, no auto-restart

        wrench::Simulation::sleep(10);

        // Create a VM on the Cloud Service
        auto cloud_service = this->test->compute_service;
        auto vm_name = cloud_service->createVM(1, this->test->task->getMemoryRequirement());
        auto vm_cs = cloud_service->startVM(vm_name);

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a standard job
        auto job = job_manager->createStandardJob(this->test->task, {{this->test->input_file, wrench::FileLocation::LOCATION(this->test->storage_service, this->test->input_file)},
                                                                     {this->test->output_file, wrench::FileLocation::LOCATION(this->test->storage_service, this->test->output_file)}});

        // Submit the standard job to the compute service, making it sure it runs on FailedHost1
        job_manager->submitJob(job, vm_cs);

        // Wait for a workflow execution event
        std::shared_ptr<wrench::ExecutionEvent> event = this->waitForNextEvent();
        if (not std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event!");
        }

        auto real_event = std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event);
        auto cause = std::dynamic_pointer_cast<wrench::HostError>(real_event->failure_cause);
        if (not cause) {
            throw std::runtime_error("Invalid failure cause type: " + real_event->failure_cause->toString() + " (expected: HostError)");
        }

        // Without this sleep, due to instantaneous zero-byte messages,
        // the cloud compute service may not yet "see" the VM as down
        // even though we got the event that the job has failed
        wrench::Simulation::sleep(1);

        // Check that the VM is down
        if (not cloud_service->isVMDown(vm_name)) {
            throw std::runtime_error("The VM should be down!");
        }

        // Restart the VM (which shouldn't work because the Cloud has no more resources right now)
        try {
            cloud_service->startVM(vm_name);
            throw std::runtime_error("Should not be able to restart VM since the CloudComputeService no longer has resources!");
        } catch (wrench::ExecutionException &e) {
            // expected
        }

        return 0;
    }
};

TEST_F(CloudServiceHostFailuresTest, FailureOfAVMWithRunningJob) {
    DO_TEST_WITH_FORK(do_CloudServiceFailureOfAVMWithRunningJob_test);
}

void CloudServiceHostFailuresTest::do_CloudServiceFailureOfAVMWithRunningJob_test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");
    //            argv[2] = strdup("--wrench-full-log");


    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string stable_host = "StableHost";
    std::vector<std::string> compute_hosts;
    compute_hosts.emplace_back("FailedHost1");

    // Create a Compute Service that has access to two hosts
    compute_service = simulation->add(
            new wrench::CloudComputeService(stable_host,
                                            compute_hosts,
                                            "/scratch",
                                            {}));

    // Create a Storage Service
    storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(stable_host, {"/"},
                                                                                               {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "10MB"}}));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    wms = simulation->add(new CloudServiceFailureOfAVMTestWMS(this, stable_host));

    // Staging the input_file on the storage service
    // Create a File Registry Service
    simulation->add(new wrench::FileRegistryService(stable_host));
    storage_service->createFile(this->input_file);

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**    FAILURE WITH RUNNING JOB FOLLOWED BY RESTART                  **/
/**********************************************************************/

class CloudServiceFailureOfAVMAndRestartTestWMS : public wrench::ExecutionController {

public:
    CloudServiceFailureOfAVMAndRestartTestWMS(CloudServiceHostFailuresTest *test,
                                              std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    CloudServiceHostFailuresTest *test;

    int main() override {

        // Starting a FailedHost1 murderer!!
        auto murderer = std::make_shared<wrench::ResourceSwitcher>("StableHost", 100, "FailedHost1",
                                                                   wrench::ResourceSwitcher::Action::TURN_OFF, wrench::ResourceSwitcher::ResourceType::HOST);
        murderer->setSimulation(this->getSimulation());
        murderer->start(murderer, true, false);// Daemonized, no auto-restart

        // Starting a FailedHost1 resurector!!
        auto resurector = std::make_shared<wrench::ResourceSwitcher>("StableHost", 1000, "FailedHost1",
                                                                     wrench::ResourceSwitcher::Action::TURN_ON, wrench::ResourceSwitcher::ResourceType::HOST);
        resurector->setSimulation(this->getSimulation());
        resurector->start(resurector, true, false);// Daemonized, no auto-restart

        wrench::Simulation::sleep(10);

        // Create a VM on the Cloud Service
        auto cloud_service = this->test->compute_service;
        auto vm_name = cloud_service->createVM(1, this->test->task->getMemoryRequirement());
        auto vm_cs = cloud_service->startVM(vm_name);

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a standard job
        auto job = job_manager->createStandardJob(this->test->task,
                                                  {{this->test->input_file, wrench::FileLocation::LOCATION(this->test->storage_service, this->test->input_file)},
                                                   {this->test->output_file, wrench::FileLocation::LOCATION(this->test->storage_service, this->test->output_file)}});

        // Submit the standard job to the compute service, making it sure it runs on FailedHost1
        job_manager->submitJob(job, vm_cs);

        // Wait for a workflow execution event
        auto event = this->waitForNextEvent();
        if (not std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event!");
        }
        auto real_event = std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event);
        auto cause = std::dynamic_pointer_cast<wrench::HostError>(real_event->failure_cause);
        if (not cause) {
            throw std::runtime_error("Invalid failure cause: " + real_event->failure_cause->toString() + " (expected: HostError)");
        }

        // Without this sleep, due to instantaneous zero-byte messages,
        // the cloud compute service may not yet "see" the VM as down
        // even though we got the event that the job has failed
        wrench::Simulation::sleep(1);

        // Check that the VM is down
        if (not cloud_service->isVMDown(vm_name)) {
            throw std::runtime_error("The VM should be down!");
        }

        // Restart the VM (which shouldn't work because the Cloud has no more resources right now)
        try {
            cloud_service->startVM(vm_name);
            throw std::runtime_error("Should not be able to restart VM since the CloudComputeService no longer has resources!");
        } catch (wrench::ExecutionException &e) {
            // expected
        }

        wrench::Simulation::sleep(2000);

        job = job_manager->createStandardJob(this->test->task,
                                             {{this->test->input_file, wrench::FileLocation::LOCATION(this->test->storage_service, this->test->input_file)},
                                              {this->test->output_file, wrench::FileLocation::LOCATION(this->test->storage_service, this->test->output_file)}});

        auto vm_cs2 = cloud_service->startVM(vm_name);
        // Submit the standard job to the compute service, making it sure it runs on FailedHost1
        job_manager->submitJob(job, vm_cs2);

        // Wait for a workflow execution event
        auto event2 = this->waitForNextEvent();
        if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event2)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event2->toString());
        }


        return 0;
    }
};

TEST_F(CloudServiceHostFailuresTest, FailureOfAVMAndRestartWithRunningJobAndRestart) {
    DO_TEST_WITH_FORK(do_CloudServiceFailureOfAVMWithRunningJobFollowedByRestart_test);
}

void CloudServiceHostFailuresTest::do_CloudServiceFailureOfAVMWithRunningJobFollowedByRestart_test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");

    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string stable_host = "StableHost";
    std::vector<std::string> compute_hosts;
    compute_hosts.emplace_back("FailedHost1");

    // Create a Compute Service that has access to two hosts
    compute_service = simulation->add(
            new wrench::CloudComputeService(stable_host,
                                            compute_hosts,
                                            "/scratch",
                                            {}));

    // Create a Storage Service
    storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(stable_host, {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "10MB"}}));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    simulation->add(new CloudServiceFailureOfAVMAndRestartTestWMS(this, stable_host));

    // Staging the input_file on the storage service
    // Create a File Registry Service
    simulation->add(new wrench::FileRegistryService(stable_host));
    storage_service->createFile(input_file);

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**                    RANDOM FAILURES                               **/
/**********************************************************************/

class CloudServiceRandomFailuresTestWMS : public wrench::ExecutionController {

public:
    CloudServiceRandomFailuresTestWMS(CloudServiceHostFailuresTest *test,
                                      std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    CloudServiceHostFailuresTest *test;

    int main() override {

        // Create a job manager
        auto job_manager = this->createJobManager();

        unsigned long NUM_TRIALS = 250;

        auto cloud_service = this->test->compute_service;

        for (unsigned long trial = 0; trial < NUM_TRIALS; trial++) {

            WRENCH_INFO("*** Trial %ld", trial);

            // Starting a FailedHost1 random repeat switch!!
            unsigned long seed1 = trial * 2 + 37;
            auto switch1 = std::make_shared<wrench::ResourceRandomRepeatSwitcher>(
                    "StableHost", seed1, 10, 100, 10, 100,
                    "FailedHost1", wrench::ResourceRandomRepeatSwitcher::ResourceType::HOST);
            switch1->setSimulation(this->getSimulation());
            switch1->start(switch1, true, false);// Daemonized, no auto-restart

            // Starting a FailedHost2 random repeat switch!!
            unsigned long seed2 = trial * 17 + 42;
            auto switch2 = std::make_shared<wrench::ResourceRandomRepeatSwitcher>(
                    "StableHost", seed1, 10, 100, 10, 100,
                    "FailedHost2", wrench::ResourceRandomRepeatSwitcher::ResourceType::HOST);
            switch2->setSimulation(this->getSimulation());
            switch2->start(switch1, true, false);// Daemonized, no auto-restart

            // Add a task1 to the workflow
            auto task = this->test->workflow->addTask("task_" + std::to_string(trial), 50, 1, 1, 0);
            auto output_file = wrench::Simulation::addFile("output_file_" + std::to_string(trial), 2000);

            task->addInputFile(this->test->input_file);
            task->addOutputFile(output_file);

            std::shared_ptr<wrench::ExecutionEvent> event = nullptr;
            unsigned long total_num_vm_start_attempts = 0;
            unsigned long num_vm_start_attempts = 0;
            unsigned long num_job_submission_attempts = 0;
            bool completed;
            do {

                if (num_vm_start_attempts > 200) exit(0);

                // Create a standard job
                auto job = job_manager->createStandardJob(task, {{this->test->input_file,
                                                                  wrench::FileLocation::LOCATION(this->test->storage_service, this->test->input_file)},
                                                                 {output_file, wrench::FileLocation::LOCATION(this->test->storage_service, output_file)}});

                // Create and Start a VM (sleep 10 and retry if unsuccessful)
                std::shared_ptr<wrench::BareMetalComputeService> vm_cs;
                std::string vm_name;
                try {
                    // WRENCH_INFO("Trying to start the VM");
                    // Create and start a VM
                    num_vm_start_attempts++;
                    total_num_vm_start_attempts++;
                    vm_name = cloud_service->createVM(task->getMinNumCores(), task->getMemoryRequirement());
                    vm_cs = cloud_service->startVM(vm_name);
                } catch (wrench::ExecutionException &e) {
                    wrench::Simulation::sleep(10);
                    continue;
                }
                //                                WRENCH_INFO("*** WAS ABLE TO START THE VM AFTER %lu attempts", num_vm_start_attempts);
                num_vm_start_attempts = 0;

                // Submit the standard job to the compute service, making it sure it runs on FailedHost1
                job_manager->submitJob(job, vm_cs);
                num_job_submission_attempts++;

                // Wait for a workflow execution event
                event = this->waitForNextEvent();

                completed = (std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event) != nullptr);
                if (completed) {
                    cloud_service->shutdownVM(vm_name);
                    cloud_service->destroyVM(vm_name);
                }

            } while (not completed);

            WRENCH_INFO("*** WAS ABLE TO RUN THE JOB AFTER %lu attempts (%lu VM start attempts)",
                        num_job_submission_attempts,
                        total_num_vm_start_attempts);
            switch1->kill();
            switch2->kill();

            wrench::Simulation::sleep(10.0);
            this->test->workflow->removeTask(task);
            this->test->workflow->removeFile(output_file);
        }

        return 0;
    }
};

TEST_F(CloudServiceHostFailuresTest, RandomFailures) {
    DO_TEST_WITH_FORK(do_CloudServiceRandomFailures_test);
}

void CloudServiceHostFailuresTest::do_CloudServiceRandomFailures_test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 3;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");
    argv[2] = strdup("--cfg=contexts/stack-size:100");
    //        argv[3] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string stable_host = "StableHost";
    std::vector<std::string> compute_hosts;
    compute_hosts.emplace_back("FailedHost1");
    compute_hosts.emplace_back("FailedHost2");

    // Create a Compute Service that has access to two hosts
    compute_service = simulation->add(
            new wrench::CloudComputeService(stable_host,
                                            compute_hosts,
                                            "/scratch",
                                            {}));

    // Create a Storage Service
    storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(stable_host, {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "10MB"}}));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    wms = simulation->add(new CloudServiceRandomFailuresTestWMS(this, stable_host));

    // Staging the input_file on the storage service
    // Create a File Registry Service
    simulation->add(new wrench::FileRegistryService(stable_host));
    storage_service->createFile(input_file);

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
