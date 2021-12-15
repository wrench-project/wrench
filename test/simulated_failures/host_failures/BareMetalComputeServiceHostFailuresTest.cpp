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

WRENCH_LOG_CATEGORY(bare_metal_compute_service_host_failures_test, "Log category for BareMetalComputeServiceSimulatedHostFailuresTests");


class BareMetalComputeServiceHostFailuresTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::Workflow> workflow;

    std::shared_ptr<wrench::DataFile> input_file;
    std::shared_ptr<wrench::DataFile> output_file;
    std::shared_ptr<wrench::WorkflowTask> task;
    std::shared_ptr<wrench::StorageService> storage_service = nullptr;
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;

    void do_BareMetalComputeServiceOneFailureCausingWorkUnitRestartOnAnotherHost_test();
    void do_BareMetalComputeServiceOneFailureCausingWorkUnitRestartOnSameHost_test();
    void do_BareMetalComputeServiceRandomFailures_test();
    void do_BareMetalComputeServiceFailureOnServiceThatTerminatesWhenAllItsResourcesAreDown_test();

protected:

    ~BareMetalComputeServiceHostFailuresTest() {
        workflow->clear();
    }

    BareMetalComputeServiceHostFailuresTest() {
        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();


        // Create two files
        input_file = workflow->addFile("input_file", 10000.0);
        output_file = workflow->addFile("output_file", 20000.0);

        // Create one task1
        task = workflow->addTask("task1", 3600, 1, 1, 0);
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
/**                FAIL OVER TO SECOND HOST  TEST                    **/
/**********************************************************************/

class BareMetalComputeServiceOneFailureCausingWorkUnitRestartOnAnotherHostTestWMS : public wrench::ExecutionController {

public:
    BareMetalComputeServiceOneFailureCausingWorkUnitRestartOnAnotherHostTestWMS(BareMetalComputeServiceHostFailuresTest *test,
                                                                                std::shared_ptr<wrench::Workflow> workflow,
                                                                                std::string &hostname, std::shared_ptr<wrench::ComputeService> cs,
                                                                                std::shared_ptr<wrench::StorageService> ss) :
            wrench::ExecutionController(workflow, nullptr, nullptr, {cs}, {ss}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    BareMetalComputeServiceHostFailuresTest *test;

    int main() override {

        // Starting a FailedHost1 murderer!!
        auto murderer = std::shared_ptr<wrench::ResourceSwitcher>(new wrench::ResourceSwitcher("StableHost", 100, "FailedHost1",
                                                                                               wrench::ResourceSwitcher::Action::TURN_OFF, wrench::ResourceSwitcher::ResourceType::HOST));
        murderer->setSimulation(this->simulation);
        murderer->start(murderer, true, false); // Daemonized, no auto-restart

        // Starting a FailedHost1 resurector!!
        auto resurector = std::shared_ptr<wrench::ResourceSwitcher>(new wrench::ResourceSwitcher("StableHost", 1000, "FailedHost1",
                                                                                                 wrench::ResourceSwitcher::Action::TURN_ON, wrench::ResourceSwitcher::ResourceType::HOST));
        resurector->setSimulation(this->simulation);
        resurector->start(resurector, true, false); // Daemonized, no auto-restart


        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a standard job
        auto job = job_manager->createStandardJob(this->test->task,
                                                  {{this->test->input_file, wrench::FileLocation::LOCATION(this->test->storage_service)},
                                                   {this->test->output_file, wrench::FileLocation::LOCATION(this->test->storage_service)}});

        // Submit the standard job to the compute service, making it sure it runs on FailedHost1
        std::map<std::string, std::string> service_specific_args;
        service_specific_args["task1"] = "FailedHost1";
        job_manager->submitJob(job, this->test->compute_service, service_specific_args);

        // Wait for a workflow execution event
        std::shared_ptr<wrench::ExecutionEvent> event = this->waitForNextEvent();
        if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event!");
        }

        // Paranoid check
        if (not wrench::StorageService::lookupFile(this->test->output_file,
                                                   wrench::FileLocation::LOCATION(this->test->storage_service))) {
            throw std::runtime_error("Output file not written to storage service");
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceHostFailuresTest, OneFailureCausingWorkUnitRestartOnAnotherHost) {
    DO_TEST_WITH_FORK(do_BareMetalComputeServiceOneFailureCausingWorkUnitRestartOnAnotherHost_test);
}

void BareMetalComputeServiceHostFailuresTest::do_BareMetalComputeServiceOneFailureCausingWorkUnitRestartOnAnotherHost_test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");
//    argv[2] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string stable_host = "StableHost";

    // Create a Compute Service that has access to two hosts
    compute_service = simulation->add(
            new wrench::BareMetalComputeService(stable_host,
                                                (std::map<std::string, std::tuple<unsigned long, double>>){
                                                        std::make_pair("FailedHost1", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)),
                                                        std::make_pair("FailedHost2", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))
                                                },
                                                "/scratch",
                                                {
                                                 {wrench::BareMetalComputeServiceProperty::FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH, "false"}}));

    // Create a Storage Service
    storage_service = simulation->add(new wrench::SimpleStorageService(stable_host, {"/"}));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;;
    wms = simulation->add(new BareMetalComputeServiceOneFailureCausingWorkUnitRestartOnAnotherHostTestWMS(this, workflow, stable_host, compute_service, storage_service));

    // Staging the input_file on the storage service
    // Create a File Registry Service
    simulation->add(new wrench::FileRegistryService(stable_host));
    simulation->stageFile(input_file, storage_service);

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}



/**********************************************************************/
/**                    RESTART ON SAME HOST                          **/
/**********************************************************************/

class BareMetalComputeServiceOneFailureCausingWorkUnitRestartOnSameHostTestWMS : public wrench::ExecutionController {

public:
    BareMetalComputeServiceOneFailureCausingWorkUnitRestartOnSameHostTestWMS(BareMetalComputeServiceHostFailuresTest *test,
                                                                             std::shared_ptr<wrench::Workflow> workflow,
                                                                             std::string &hostname,
                                                                             std::shared_ptr<wrench::ComputeService> cs,
                                                                             std::shared_ptr<wrench::StorageService> ss) :
            wrench::ExecutionController(workflow, nullptr, nullptr, {cs}, {ss}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    BareMetalComputeServiceHostFailuresTest *test;

    int main() override {

        // Starting a FailedHost1 murderer!!
        auto murderer = std::shared_ptr<wrench::ResourceSwitcher>(new wrench::ResourceSwitcher("StableHost", 100, "FailedHost1",
                                                                                               wrench::ResourceSwitcher::Action::TURN_OFF, wrench::ResourceSwitcher::ResourceType::HOST));
        murderer->setSimulation(this->simulation);
        murderer->start(murderer, true, false); // Daemonized, no auto-restart

        // Starting a FailedHost1 resurector!!
        auto resurector = std::shared_ptr<wrench::ResourceSwitcher>(new wrench::ResourceSwitcher("StableHost", 1000, "FailedHost1",
                                                                                                 wrench::ResourceSwitcher::Action::TURN_ON, wrench::ResourceSwitcher::ResourceType::HOST));
        resurector->setSimulation(this->simulation);
        resurector->start(resurector, true, false); // Daemonized, no auto-restart


        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a standard job
        auto job = job_manager->createStandardJob(this->test->task, {{this->test->input_file, wrench::FileLocation::LOCATION(this->test->storage_service)},
                                                                     {this->test->output_file, wrench::FileLocation::LOCATION(this->test->storage_service)}});

        // Submit the standard job to the compute service, making it sure it runs on FailedHost1
        job_manager->submitJob(job, this->test->compute_service, {});

        // Wait for a workflow execution event
        std::shared_ptr<wrench::ExecutionEvent> event = this->waitForNextEvent();
        if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event!");
        }

        // Paranoid check
        if (not wrench::StorageService::lookupFile(this->test->output_file,
                                                   wrench::FileLocation::LOCATION(this->test->storage_service))) {
            throw std::runtime_error("Output file not written to storage service");
        }



        return 0;
    }
};

TEST_F(BareMetalComputeServiceHostFailuresTest, OneFailureCausingWorkUnitRestartOnSameHost) {
    DO_TEST_WITH_FORK(do_BareMetalComputeServiceOneFailureCausingWorkUnitRestartOnSameHost_test);
}

void BareMetalComputeServiceHostFailuresTest::do_BareMetalComputeServiceOneFailureCausingWorkUnitRestartOnSameHost_test() {

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

    // Create a Compute Service that has access to one hosts
    compute_service = simulation->add(
            new wrench::BareMetalComputeService(stable_host,
                                                (std::map<std::string, std::tuple<unsigned long, double>>){
                                                        std::make_pair("FailedHost1", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)),
                                                },
                                                "/scratch",
                                                {{wrench::BareMetalComputeServiceProperty::FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH, "false"},
                                                 }));

    // Create a Storage Service
    storage_service = simulation->add(new wrench::SimpleStorageService(stable_host, {"/"}));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;;
    wms = simulation->add(new BareMetalComputeServiceOneFailureCausingWorkUnitRestartOnSameHostTestWMS(this, workflow, stable_host, compute_service, storage_service));

    // Staging the input_file on the storage service
    // Create a File Registry Service
    simulation->add(new wrench::FileRegistryService(stable_host));
    simulation->stageFile(input_file, storage_service);

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**                    RANDOM FAILURES                               **/
/**********************************************************************/

class BareMetalComputeServiceRandomFailuresTestWMS : public wrench::ExecutionController {

public:
    BareMetalComputeServiceRandomFailuresTestWMS(BareMetalComputeServiceHostFailuresTest *test,
                                                 std::shared_ptr<wrench::Workflow> workflow,
                                                 std::string &hostname,
                                                 std::shared_ptr<wrench::ComputeService> cs,
                                                 std::shared_ptr<wrench::StorageService> ss) :
            wrench::ExecutionController(workflow, nullptr, nullptr, {cs}, {ss}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    BareMetalComputeServiceHostFailuresTest *test;

    int main() override {

        // Create a job manager
        auto job_manager = this->createJobManager();

        unsigned long NUM_TRIALS = 500;

        for (unsigned long trial=0; trial < NUM_TRIALS; trial++) {

            WRENCH_INFO("*** Trial %ld", trial);

            // Starting a FailedHost1 random repeat switch!!
            unsigned long seed1 = trial * 2 + 37;
            auto switch1 = std::shared_ptr<wrench::ResourceRandomRepeatSwitcher>(
                    new wrench::ResourceRandomRepeatSwitcher("StableHost", seed1, 10, 100, 10, 100,
                                                             "FailedHost1", wrench::ResourceRandomRepeatSwitcher::ResourceType::HOST));
            switch1->setSimulation(this->simulation);
            switch1->start(switch1, true, false); // Daemonized, no auto-restart

            // Starting a FailedHost2 random repeat switch!!
            unsigned long seed2 = trial * 7 + 417;
            auto switch2 = std::shared_ptr<wrench::ResourceRandomRepeatSwitcher>(
                    new wrench::ResourceRandomRepeatSwitcher("StableHost", seed2, 10, 100, 10, 100,
                                                             "FailedHost2", wrench::ResourceRandomRepeatSwitcher::ResourceType::HOST));
            switch2->setSimulation(this->simulation);
            switch2->start(switch2, true, false); // Daemonized, no auto-restart

            // Add a task1 to the workflow
            auto task = this->test->workflow->addTask("task_" + std::to_string(trial), 80, 1, 1, 0);
            auto output_file = this->test->workflow->addFile("output_file_" + std::to_string(trial), 20000.0);
            task->addInputFile(this->test->input_file);
            task->addOutputFile(output_file);

            // Create a standard job
            auto job = job_manager->createStandardJob(task, {{this->test->input_file,
                                                                     wrench::FileLocation::LOCATION(this->test->storage_service)},
                                                             {output_file,
                                                                     wrench::FileLocation::LOCATION(this->test->storage_service)}});

            // Submit the standard job to the compute service, making it sure it runs on FailedHost1
            job_manager->submitJob(job, this->test->compute_service, {});

            // Wait for a workflow execution event
            std::shared_ptr<wrench::ExecutionEvent> event = this->waitForNextEvent();
            if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                throw std::runtime_error("Unexpected workflow execution event!");
            }

            switch1->kill();
            switch2->kill();

            wrench::Simulation::sleep(10.0);
            this->test->workflow->removeTask(task);
            this->test->workflow->removeFile(output_file);

        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceHostFailuresTest, RandomFailures) {
    DO_TEST_WITH_FORK(do_BareMetalComputeServiceRandomFailures_test);
}

void BareMetalComputeServiceHostFailuresTest::do_BareMetalComputeServiceRandomFailures_test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");
//    argv[2] = strdup("--wrench-full-logs");


    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string stable_host = "StableHost";

    // Create a Compute Service that has access to two hosts
    compute_service = simulation->add(
            new wrench::BareMetalComputeService(stable_host,
                                                (std::map<std::string, std::tuple<unsigned long, double>>){
                                                        std::make_pair("FailedHost1", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)),
                                                        std::make_pair("FailedHost2", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)),
                                                },
                                                "/scratch",
                                                {
                                                 {wrench::BareMetalComputeServiceProperty::FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH, "false"}}));

    // Create a Storage Service
    storage_service = simulation->add(new wrench::SimpleStorageService(stable_host, {"/"}));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;;
    wms = simulation->add(new BareMetalComputeServiceRandomFailuresTestWMS(this, workflow, stable_host, compute_service, storage_service));

    // Staging the input_file on the storage service
    // Create a File Registry Service
    simulation->add(new wrench::FileRegistryService(stable_host));
    simulation->stageFile(input_file, (storage_service));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}



/**********************************************************************/
/**    FALURE ON SERVICE THAT DIES WHEN ALL RESOURCES ARE DOWN       **/
/**********************************************************************/

class BareMetalComputeServiceFailureOnServiceThatTerminatesWhenAllItsResourcesAreDownTestWMS : public wrench::ExecutionController {

public:
    BareMetalComputeServiceFailureOnServiceThatTerminatesWhenAllItsResourcesAreDownTestWMS(BareMetalComputeServiceHostFailuresTest *test,
                                                                                           std::shared_ptr<wrench::Workflow> workflow,
                                                                                           std::string &hostname, std::shared_ptr<wrench::ComputeService> cs,
                                                                                           std::shared_ptr<wrench::StorageService> ss) :
            wrench::ExecutionController(workflow, nullptr, nullptr, {cs}, {ss}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    BareMetalComputeServiceHostFailuresTest *test;

    int main() override {

        // Starting a FailedHost1 murderer!!
        auto murderer = std::shared_ptr<wrench::ResourceSwitcher>(new wrench::ResourceSwitcher("StableHost", 100, "FailedHost1",
                                                                                               wrench::ResourceSwitcher::Action::TURN_OFF, wrench::ResourceSwitcher::ResourceType::HOST));
        murderer->setSimulation(this->simulation);
        murderer->start(murderer, true, false); // Daemonized, no auto-restart


        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a standard job
        auto job = job_manager->createStandardJob(this->test->task, {{this->test->input_file,
                                                                             wrench::FileLocation::LOCATION(this->test->storage_service)},
                                                                     {this->test->output_file,
                                                                             wrench::FileLocation::LOCATION(this->test->storage_service)}});

        // Submit the standard job to the compute service, making it sure it runs on FailedHost1
        job_manager->submitJob(job, this->test->compute_service, {});

        // Wait for a workflow execution event
        std::shared_ptr<wrench::ExecutionEvent> event = this->waitForNextEvent();
        if (not std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event!");
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceHostFailuresTest, FailureOnServiceThatTerminatesWhenAllItsResourcesAreDown) {
    DO_TEST_WITH_FORK(do_BareMetalComputeServiceFailureOnServiceThatTerminatesWhenAllItsResourcesAreDown_test);
}

void BareMetalComputeServiceHostFailuresTest::do_BareMetalComputeServiceFailureOnServiceThatTerminatesWhenAllItsResourcesAreDown_test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");
//    argv[2] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string stable_host = "StableHost";

    // Create a Compute Service that has access to one hosts
    compute_service = simulation->add(
            new wrench::BareMetalComputeService(
                    stable_host,
                    (std::map<std::string, std::tuple<unsigned long, double>>){
                            std::make_pair("FailedHost1", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)),
                    },
                    "/scratch",
                    {
                            {wrench::BareMetalComputeServiceProperty::FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH, "true"}
                    }));

    // Create a Storage Service
    storage_service = simulation->add(new wrench::SimpleStorageService(stable_host, {"/"}));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;;
    wms = simulation->add(new BareMetalComputeServiceFailureOnServiceThatTerminatesWhenAllItsResourcesAreDownTestWMS(this, workflow, stable_host, compute_service, storage_service));

    // Staging the input_file on the storage service
    // Create a File Registry Service
    simulation->add(new wrench::FileRegistryService(stable_host));
    simulation->stageFile(input_file, storage_service);

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
