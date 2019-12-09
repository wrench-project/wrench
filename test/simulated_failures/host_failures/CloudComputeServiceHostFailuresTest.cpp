
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

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"
#include "../failure_test_util/ResourceSwitcher.h"
#include "wrench/services/helpers/ServiceTerminationDetector.h"
#include "../failure_test_util/SleeperVictim.h"
#include "../failure_test_util/ResourceRandomRepeatSwitcher.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(cloud_compute_service_host_failures_test, "Log category for CloudServiceHostFailuresTest");


class CloudServiceHostFailuresTest : public ::testing::Test {

public:
    wrench::Workflow *workflow;
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr;

    wrench::WorkflowFile *input_file;
    wrench::WorkflowFile *output_file;
    wrench::WorkflowTask *task;
    std::shared_ptr<wrench::StorageService> storage_service = nullptr;
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;

    void do_CloudServiceFailureOfAVMWithRunningJob_test();
    void do_CloudServiceFailureOfAVMWithRunningJobFollowedByRestart_test();
    void do_CloudServiceRandomFailures_test();

protected:

    CloudServiceHostFailuresTest() {
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

class CloudServiceFailureOfAVMTestWMS : public wrench::WMS {

public:
    CloudServiceFailureOfAVMTestWMS(CloudServiceHostFailuresTest *test,
                                    std::string &hostname, std::shared_ptr<wrench::ComputeService> cs,
                                    std::shared_ptr<wrench::StorageService> ss) :
            wrench::WMS(nullptr, nullptr, {cs}, {ss}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    CloudServiceHostFailuresTest *test;

    int main() override {

        // Starting a FailedHost1 murderer!!
        auto murderer = std::shared_ptr<wrench::ResourceSwitcher>(new wrench::ResourceSwitcher("StableHost", 100, "FailedHost1",
                                                                                               wrench::ResourceSwitcher::Action::TURN_OFF, wrench::ResourceSwitcher::ResourceType::HOST));
        murderer->simulation = this->simulation;
        murderer->start(murderer, true, false); // Daemonized, no auto-restart

        wrench::Simulation::sleep(10);

        // Create a VM on the Cloud Service
        auto cloud_service =  *(this->getAvailableComputeServices<wrench::CloudComputeService>().begin());
        auto vm_name = cloud_service->createVM(1, this->test->task->getMemoryRequirement());
        auto vm_cs = cloud_service->startVM(vm_name);

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a standard job
        auto job = job_manager->createStandardJob(this->test->task, {{this->test->input_file, wrench::FileLocation::LOCATION(this->test->storage_service)},
                                                                     {this->test->output_file, wrench::FileLocation::LOCATION(this->test->storage_service)}});

        // Submit the standard job to the compute service, making it sure it runs on FailedHost1
        job_manager->submitJob(job, vm_cs);

        // Wait for a workflow execution event
        std::shared_ptr<wrench::WorkflowExecutionEvent> event = this->getWorkflow()->waitForNextExecutionEvent();
        if (not std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event!");
        }
        auto real_event = std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event);
        auto cause = std::dynamic_pointer_cast<wrench::JobKilled>(real_event->failure_cause);
        if (not cause) {
            throw std::runtime_error("Invalid failure cause type: " + real_event->failure_cause->toString() + " (expected: JobKilled)");
        }
        if (cause->getJob() != job) {
            throw std::runtime_error("Failure cause does not point to the correct job");
        }
        if (cause->getComputeService() != vm_cs) {
            throw std::runtime_error("Failure cause does not point to the correct compute service");
        }

        // Check that the VM is down
        if (not cloud_service->isVMDown(vm_name)) {
            throw std::runtime_error("The VM should be down!");
        }

        // Restart the VM (which shouldn't work because the Cloud has no more resources right now)
        try {
            cloud_service->startVM(vm_name);
            throw std::runtime_error("Should not be able to restart VM since the CloudComputeService no longer has resources!");
        } catch (wrench::WorkflowExecutionException &e) {
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
            new wrench::CloudComputeService(stable_host,
                                            compute_hosts,
                                            "/scratch",
                                            {}));

    // Create a Storage Service
    storage_service = simulation->add(new wrench::SimpleStorageService(stable_host, {"/"}));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    wms = simulation->add(new CloudServiceFailureOfAVMTestWMS(this, stable_host, compute_service, storage_service));

    wms->addWorkflow(workflow);

    // Staging the input_file on the storage service
    // Create a File Registry Service
    simulation->add(new wrench::FileRegistryService(stable_host));
    simulation->stageFile(this->input_file, storage_service);

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}






/**********************************************************************/
/**    FAILURE WITH RUNNING JOB FOLLOWED BY RESTART                  **/
/**********************************************************************/

class CloudServiceFailureOfAVMAndRestartTestWMS : public wrench::WMS {

public:
    CloudServiceFailureOfAVMAndRestartTestWMS(CloudServiceHostFailuresTest *test,
                                              std::string &hostname, std::shared_ptr<wrench::ComputeService> cs, std::shared_ptr<wrench::StorageService> ss) :
            wrench::WMS(nullptr, nullptr, {cs}, {ss}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    CloudServiceHostFailuresTest *test;

    int main() override {

        // Starting a FailedHost1 murderer!!
        auto murderer = std::shared_ptr<wrench::ResourceSwitcher>(new wrench::ResourceSwitcher("StableHost", 100, "FailedHost1",
                                                                                               wrench::ResourceSwitcher::Action::TURN_OFF, wrench::ResourceSwitcher::ResourceType::HOST));
        murderer->simulation = this->simulation;
        murderer->start(murderer, true, false); // Daemonized, no auto-restart

        // Starting a FailedHost1 resurector!!
        auto resurector = std::shared_ptr<wrench::ResourceSwitcher>(new wrench::ResourceSwitcher("StableHost", 1000, "FailedHost1",
                                                                                                 wrench::ResourceSwitcher::Action::TURN_ON, wrench::ResourceSwitcher::ResourceType::HOST));
        resurector->simulation = this->simulation;
        resurector->start(resurector, true, false); // Daemonized, no auto-restart

        wrench::Simulation::sleep(10);

        // Create a VM on the Cloud Service
        auto cloud_service =  *(this->getAvailableComputeServices<wrench::CloudComputeService>().begin());
        auto vm_name = cloud_service->createVM(1, this->test->task->getMemoryRequirement());
        auto vm_cs = cloud_service->startVM(vm_name);

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a standard job
        auto job = job_manager->createStandardJob(this->test->task,
                                                  {{this->test->input_file, wrench::FileLocation::LOCATION(this->test->storage_service)},
                                                   {this->test->output_file, wrench::FileLocation::LOCATION(this->test->storage_service)}});

        // Submit the standard job to the compute service, making it sure it runs on FailedHost1
        job_manager->submitJob(job, vm_cs);

        // Wait for a workflow execution event
        auto event = this->getWorkflow()->waitForNextExecutionEvent();
        if (not std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event!");
        }
        auto real_event = std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event);
        auto cause = std::dynamic_pointer_cast<wrench::JobKilled>(real_event->failure_cause);
        if (not cause) {
            throw std::runtime_error("Invalid failure cause: " + real_event->failure_cause->toString() + " (expected: JobKilled)");
        }
        if (cause->getJob() != job) {
            throw std::runtime_error("Failure cause does not point to the correct job");
        }
        if (cause->getComputeService() != vm_cs) {
            throw std::runtime_error("Failure cause does not point to the correct compute service");
        }

        // Check that the VM is down
        if (not cloud_service->isVMDown(vm_name)) {
            throw std::runtime_error("The VM should be down!");
        }

        // Restart the VM (which shouldn't work because the Cloud has no more resources right now)
        try {
            cloud_service->startVM(vm_name);
            throw std::runtime_error("Should not be able to restart VM since the CloudComputeService no longer has resources!");
        } catch (wrench::WorkflowExecutionException &e) {
            // expected
        }


        wrench::Simulation::sleep(2000);

        auto vm_cs2 = cloud_service->startVM(vm_name);
        // Submit the standard job to the compute service, making it sure it runs on FailedHost1
        job_manager->submitJob(job, vm_cs2);

        // Wait for a workflow execution event
        auto event2 = this->getWorkflow()->waitForNextExecutionEvent();
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
            new wrench::CloudComputeService(stable_host,
                                            compute_hosts,
                                            "/scratch",
                                            {}));

    // Create a Storage Service
    storage_service = simulation->add(new wrench::SimpleStorageService(stable_host, {"/"}));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    wms = simulation->add(new CloudServiceFailureOfAVMAndRestartTestWMS(this, stable_host, compute_service, storage_service));

    wms->addWorkflow(workflow);

    // Staging the input_file on the storage service
    // Create a File Registry Service
    simulation->add(new wrench::FileRegistryService(stable_host));
    simulation->stageFile(input_file, storage_service);

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}






/**********************************************************************/
/**                    RANDOM FAILURES                               **/
/**********************************************************************/

class CloudServiceRandomFailuresTestWMS : public wrench::WMS {

public:
    CloudServiceRandomFailuresTestWMS(CloudServiceHostFailuresTest *test,
                                      std::string &hostname, std::shared_ptr<wrench::ComputeService> cs,
                                      std::shared_ptr<wrench::StorageService> ss) :
            wrench::WMS(nullptr, nullptr, {cs}, {ss}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    CloudServiceHostFailuresTest *test;

    int main() override {

        // Create a job manager
        auto job_manager = this->createJobManager();

        unsigned long NUM_TRIALS = 500;

        auto cloud_service = *(this->getAvailableComputeServices<wrench::CloudComputeService>().begin());

        for (unsigned long trial=0; trial < NUM_TRIALS; trial++) {

            WRENCH_INFO("*** Trial %ld", trial);

            // Starting a FailedHost1 random repeat switch!!
            unsigned long seed1 = trial * 2 + 37;
            auto switch1 = std::shared_ptr<wrench::ResourceRandomRepeatSwitcher>(
                    new wrench::ResourceRandomRepeatSwitcher("StableHost", seed1, 10, 100, 10, 100,
                                                             "FailedHost1", wrench::ResourceRandomRepeatSwitcher::ResourceType::HOST));
            switch1->simulation = this->simulation;
            switch1->start(switch1, true, false); // Daemonized, no auto-restart

            // Starting a FailedHost2 random repeat switch!!
            unsigned long seed2 = trial * 17 + 42;
            auto switch2 = std::shared_ptr<wrench::ResourceRandomRepeatSwitcher>(
                    new wrench::ResourceRandomRepeatSwitcher("StableHost", seed1, 10, 100, 10, 100,
                                                             "FailedHost2", wrench::ResourceRandomRepeatSwitcher::ResourceType::HOST));
            switch2->simulation = this->simulation;
            switch2->start(switch1, true, false); // Daemonized, no auto-restart

            // Add a task to the workflow
            auto task = this->test->workflow->addTask("task_" + std::to_string(trial), 50, 1, 1, 1.0, 0);
            auto output_file = this->test->workflow->addFile("output_file_" + std::to_string(trial), 2000);

            task->addInputFile(this->test->input_file);
            task->addOutputFile(output_file);

            // Create a standard job
            auto job = job_manager->createStandardJob(task, {{this->test->input_file,
                                                                           wrench::FileLocation::LOCATION(this->test->storage_service)},
                                                             {output_file, wrench::FileLocation::LOCATION(this->test->storage_service)}});

            // Create a VM
            auto vm_name = cloud_service->createVM(task->getMinNumCores(), task->getMemoryRequirement());

            std::shared_ptr<wrench::WorkflowExecutionEvent> event = nullptr;
            unsigned long total_num_vm_start_attempts = 0;
            unsigned long num_vm_start_attempts = 0;
            unsigned long num_job_submission_attempts = 0;
            do {

                // Start the VM (sleep 10 and retry if unsuccessful)
                std::shared_ptr<wrench::BareMetalComputeService> vm_cs;
                try {
//                    WRENCH_INFO("Trying to start the VM");
                    num_vm_start_attempts++;
                    total_num_vm_start_attempts++;
                    vm_cs = cloud_service->startVM(vm_name);
                } catch (wrench::WorkflowExecutionException &e) {
                    wrench::Simulation::sleep(10);
                    continue;
                }
//                WRENCH_INFO("*** WAS ABLE TO START THE VM AFTER %lu attempts", num_vm_start_attempts);
                num_vm_start_attempts = 0;

                // Submit the standard job to the compute service, making it sure it runs on FailedHost1
                job_manager->submitJob(job, vm_cs);
                num_job_submission_attempts++;

                // Wait for a workflow execution event
                event = this->getWorkflow()->waitForNextExecutionEvent();
            } while ((event == nullptr) || (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)));

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
    compute_hosts.push_back("FailedHost2");

    // Create a Compute Service that has access to two hosts
    compute_service = simulation->add(
            new wrench::CloudComputeService(stable_host,
                                            compute_hosts,
                                            "/scratch",
                                            {}));

    // Create a Storage Service
    storage_service = simulation->add(new wrench::SimpleStorageService(stable_host, {"/"}));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    wms = simulation->add(new CloudServiceRandomFailuresTestWMS(this, stable_host, compute_service, storage_service));

    wms->addWorkflow(workflow);

    // Staging the input_file on the storage service
    // Create a File Registry Service
    simulation->add(new wrench::FileRegistryService(stable_host));
    simulation->stageFile(input_file, storage_service);

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}



