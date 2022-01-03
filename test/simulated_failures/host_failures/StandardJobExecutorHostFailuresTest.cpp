/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "helper_services/standard_job_executor/StandardJobExecutorMessage.h"
#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"
#include "../failure_test_util/ResourceSwitcher.h"
#include "../failure_test_util/ResourceRandomRepeatSwitcher.h"

WRENCH_LOG_CATEGORY(standard_job_executor_host_failures_test, "Log category for StandardJobExecutorHostFailuresTests");


class StandardJobExecutorHostFailuresTest : public ::testing::Test {

public:
    wrench::Workflow *workflow;
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr;

    wrench::WorkflowFile *input_file;
    wrench::WorkflowFile *output_file;
    wrench::WorkflowTask *task;
    std::shared_ptr<wrench::StorageService> storage_service = nullptr;
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;

    void do_StandardJobExecutorOneFailureCausingWorkUnitRestartOnAnotherHost_test();
    void do_StandardJobExecutorOneFailureCausingWorkUnitRestartOnSameHost_test();
    void do_StandardJobExecutorRandomFailures_test();

protected:

    StandardJobExecutorHostFailuresTest() {
        // Create the simplest workflow
        workflow_unique_ptr = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
        workflow = workflow_unique_ptr.get();

        // Create two files
        input_file = workflow->addFile("input_file", 10000.0);
        output_file = workflow->addFile("output_file", 20000.0);

        // Create one task
        task = workflow->addTask("task", 3600, 1, 1, 0);
        task->addInputFile(input_file);
        task->addOutputFile(output_file);

        // Create a platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"FailedHost1\" speed=\"1f\" core=\"1\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"30000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"101B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>  "
                          "       <host id=\"FailedHost2\" speed=\"1f\" core=\"1\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"30000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"101B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>  "
                          "       <host id=\"StableHost\" speed=\"1f\" core=\"1\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"30000B\"/>"
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

class StandardJobExecutorOneFailureCausingWorkUnitRestartOnAnotherHostTestWMS : public wrench::WMS {

public:
    StandardJobExecutorOneFailureCausingWorkUnitRestartOnAnotherHostTestWMS(StandardJobExecutorHostFailuresTest *test,
                                                                            std::string &hostname, std::shared_ptr<wrench::StorageService> ss) :
            wrench::WMS(nullptr, nullptr, {}, {ss}, {}, nullptr, hostname, "testX") {
        this->test = test;
    }

private:

    StandardJobExecutorHostFailuresTest *test;

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


        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)
        auto job = job_manager->createStandardJob(
                {this->test->task},
                {
                        {*(this->test->task->getInputFiles().begin()),  wrench::FileLocation::LOCATION(this->test->storage_service)},
                        {*(this->test->task->getOutputFiles().begin()), wrench::FileLocation::LOCATION(this->test->storage_service)}
                },
                {},
                {},
                {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(this->getWorkflow()->getFileByID("input_file"),
                                                                                             wrench::FileLocation::LOCATION(this->test->storage_service))});

        // Create a StandardJobExecutor
        std::shared_ptr<wrench::StandardJobExecutor> executor;
        try {
            executor = std::shared_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            this->simulation,
                            this->mailbox_name,
                            {"StableHost"},
                            job,
                            {std::make_pair("FailedHost1", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)),
                             std::make_pair("FailedHost2", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                            nullptr,
                            false,
                            nullptr,
                            {}, {}
                    ));
        } catch (std::invalid_argument &e) {
            throw std::runtime_error("Should be able to create the standard job executor!");
        }

        executor->start(executor, true, false); // Daemonized, no auto-restart

        // Wait for a message on my mailbox_name
        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg = std::dynamic_pointer_cast<wrench::StandardJobExecutorDoneMessage>(message);
        if (!msg) {
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        return 0;
    }
};

TEST_F(StandardJobExecutorHostFailuresTest, OneFailureCausingWorkUnitRestartOnAnotherHost) {
    DO_TEST_WITH_FORK(do_StandardJobExecutorOneFailureCausingWorkUnitRestartOnAnotherHost_test);
}

void StandardJobExecutorHostFailuresTest::do_StandardJobExecutorOneFailureCausingWorkUnitRestartOnAnotherHost_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");



    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string stable_host = "StableHost";


    // Create a Storage Service
    storage_service = simulation->add(new wrench::SimpleStorageService(stable_host, {"/"}));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    wms = simulation->add(new StandardJobExecutorOneFailureCausingWorkUnitRestartOnAnotherHostTestWMS(this, stable_host, storage_service));

    wms->addWorkflow(workflow);

    // Staging the input_file on the storage service
    // Create a File Registry Service
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(stable_host)));
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}



/**********************************************************************/
/**                    RESTART ON SAME HOST                          **/
/**********************************************************************/

class StandardJobExecutorOneFailureCausingWorkUnitRestartOnSameHostTestWMS : public wrench::WMS {

public:
    StandardJobExecutorOneFailureCausingWorkUnitRestartOnSameHostTestWMS(StandardJobExecutorHostFailuresTest *test,
                                                                         std::string &hostname, std::shared_ptr<wrench::StorageService> ss) :
            wrench::WMS(nullptr, nullptr, {}, {ss}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    StandardJobExecutorHostFailuresTest *test;

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


        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)
        auto job = job_manager->createStandardJob(
                {this->test->task},
                {
                        {*(this->test->task->getInputFiles().begin()),  wrench::FileLocation::LOCATION(this->test->storage_service)},
                        {*(this->test->task->getOutputFiles().begin()), wrench::FileLocation::LOCATION(this->test->storage_service)}
                },
                {},
                {},
                {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(this->getWorkflow()->getFileByID("input_file"),
                                                                                             wrench::FileLocation::LOCATION(this->test->storage_service))});

        // Create a StandardJobExecutor
        std::shared_ptr<wrench::StandardJobExecutor> executor;
        try {
            executor = std::shared_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            this->simulation,
                            this->mailbox_name,
                            {"StableHost"},
                            job,
                            {std::make_pair("FailedHost1", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                            nullptr,
                            false,
                            nullptr,
                            {}, {}
                    ));
        } catch (std::invalid_argument &e) {
            throw std::runtime_error("Should be able to create the standard job executor!");
        }

        executor->start(executor, true, false); // Daemonized, no auto-restart

        // Wait for a message on my mailbox_name
        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg = std::dynamic_pointer_cast<wrench::StandardJobExecutorDoneMessage>(message);
        if (!msg) {
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        return 0;
    }
};

TEST_F(StandardJobExecutorHostFailuresTest, OneFailureCausingWorkUnitRestartOnSameHost) {
    DO_TEST_WITH_FORK(do_StandardJobExecutorOneFailureCausingWorkUnitRestartOnSameHost_test);
}

void StandardJobExecutorHostFailuresTest::do_StandardJobExecutorOneFailureCausingWorkUnitRestartOnSameHost_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");



    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string stable_host = "StableHost";

    // Create a Storage Service
    storage_service = simulation->add(new wrench::SimpleStorageService(stable_host, {"/"}));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    wms = simulation->add(new StandardJobExecutorOneFailureCausingWorkUnitRestartOnSameHostTestWMS(this, stable_host, storage_service));

    wms->addWorkflow(workflow);

    // Staging the input_file on the storage service
    // Create a File Registry Service
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(stable_host)));
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**                    RANDOM FAILURES                               **/
/**********************************************************************/

class StandardJobExecutorRandomFailuresTestWMS : public wrench::WMS {

public:
    StandardJobExecutorRandomFailuresTestWMS(StandardJobExecutorHostFailuresTest *test,
                                             std::string &hostname, std::shared_ptr<wrench::StorageService> ss) :
            wrench::WMS(nullptr, nullptr, {}, {ss}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    StandardJobExecutorHostFailuresTest *test;

    int main() override {


        // Create a job manager
        auto job_manager = this->createJobManager();

        unsigned long NUM_TRIALS =500;

        for (unsigned long trial=0; trial < NUM_TRIALS; trial++) {

            WRENCH_INFO("*** Trial %ld", trial);

            // Starting a FailedHost1 random repeat switch!!
            unsigned long seed1 = trial * 3 + 137;
            auto switch1 = std::shared_ptr<wrench::ResourceRandomRepeatSwitcher>(
                    new wrench::ResourceRandomRepeatSwitcher("StableHost", seed1, 10, 100, 10, 100,
                                                             "FailedHost1", wrench::ResourceRandomRepeatSwitcher::ResourceType::HOST));
            switch1->simulation = this->simulation;
            switch1->start(switch1, true, false); // Daemonized, no auto-restart

            // Starting a FailedHost2 random repeat switch!!
            unsigned long seed2 = trial * 11 + 317;
            auto switch2 = std::shared_ptr<wrench::ResourceRandomRepeatSwitcher>(
                    new wrench::ResourceRandomRepeatSwitcher("StableHost", seed2, 10, 100, 10, 100,
                                                             "FailedHost2", wrench::ResourceRandomRepeatSwitcher::ResourceType::HOST));
            switch2->simulation = this->simulation;
            switch2->start(switch2, true, false); // Daemonized, no auto-restart

            // Add a task to the workflow
            auto task = this->test->workflow->addTask("task_" + std::to_string(trial), 80, 1, 1, 0);
            auto output_file = this->test->workflow->addFile("output_file_" + std::to_string(trial), 2000);
            task->addInputFile(this->test->input_file);
            task->addOutputFile(output_file);


            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)
            auto job = job_manager->createStandardJob(
                    {task},
                    {
                            {*(task->getInputFiles().begin()),  wrench::FileLocation::LOCATION(this->test->storage_service)},
                            {*(task->getOutputFiles().begin()), wrench::FileLocation::LOCATION(this->test->storage_service)}
                    },
                    {},
                    {},
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(
                            output_file, wrench::FileLocation::LOCATION(this->test->storage_service))});

            // Create a StandardJobExecutor
            std::shared_ptr<wrench::StandardJobExecutor> executor;
            try {
                executor = std::shared_ptr<wrench::StandardJobExecutor>(
                        new wrench::StandardJobExecutor(
                                this->simulation,
                                this->mailbox_name,
                                {"StableHost"},
                                job,
                                {
                                        std::make_pair("FailedHost1", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)),
                                        std::make_pair("FailedHost2", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))
                                },
                                nullptr,
                                false,
                                nullptr,
                                {}, {}
                        ));
            } catch (std::invalid_argument &e) {
                throw std::runtime_error("Should be able to create the standard job executor!");
            }

            executor->start(executor, true, false); // Daemonized, no auto-restart

            // Wait for a message on my mailbox_name
            std::shared_ptr<wrench::SimulationMessage> message;
            try {
                message = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
            } catch (std::shared_ptr<wrench::NetworkError> &cause) {
                throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
            }

            // Did we get the expected message?
            auto msg = std::dynamic_pointer_cast<wrench::StandardJobExecutorDoneMessage>(message);
            if (!msg) {
                throw std::runtime_error("Unexpected '" + message->getName() + "' message");
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

TEST_F(StandardJobExecutorHostFailuresTest, RandomFailures) {
    DO_TEST_WITH_FORK(do_StandardJobExecutorRandomFailures_test);
}

void StandardJobExecutorHostFailuresTest::do_StandardJobExecutorRandomFailures_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");


    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string stable_host = "StableHost";

    // Create a Storage Service
    storage_service = simulation->add(new wrench::SimpleStorageService(stable_host, {"/"}));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    wms = simulation->add(new StandardJobExecutorRandomFailuresTestWMS(this, stable_host, storage_service));

    wms->addWorkflow(workflow);

    // Staging the input_file on the storage service
    // Create a File Registry Service
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(stable_host)));
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}
