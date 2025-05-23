/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <wrench-dev.h>

#include <wrench/action/FileCopyAction.h>
#include <wrench/services/helper_services/action_executor/ActionExecutorMessage.h>
#include <wrench/services/helper_services/action_executor/ActionExecutor.h>
#include <wrench/job/CompoundJob.h>
#include <wrench/failure_causes/HostError.h>

#include <memory>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(file_copy_action_executor_test, "Log category for FileCopyActionExecutorTest");

//#define EPSILON (std::numeric_limits<double>::epsilon())
#define EPSILON (0.000001)


class FileCopyActionExecutorTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::Simulation> simulation;

    void do_FileCopyActionExecutorSuccessTest_test();
    void do_FileCopyActionExecutorSuccessSameHostTest_test();


protected:
    FileCopyActionExecutorTest() {

        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk2\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/disk2/\"/>"
                          "          </disk>"

                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host2\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host3\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host4\" speed=\"1f\" core=\"10\">  "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "         <prop id=\"ram\" value=\"1024B\"/> "
                          "       </host>  "
                          "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <link id=\"2\" bandwidth=\"0.1MBps\" latency=\"10us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host1\" dst=\"Host3\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host2\" dst=\"Host3\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host3\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host1\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
                          "   </zone> "
                          "</platform>";
        // Create a four-host 10-core platform file
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    std::shared_ptr<wrench::Workflow> workflow;

public:
    std::shared_ptr<wrench::DataFile> file;
    std::shared_ptr<wrench::StorageService> ss1;
    std::shared_ptr<wrench::StorageService> ss2;
};


/**********************************************************************/
/**  DO FILE_READ ACTION EXECUTOR SUCCESS TEST                       **/
/**********************************************************************/

class FileCopyActionExecutorSuccessTestWMS : public wrench::ExecutionController {

public:
    FileCopyActionExecutorSuccessTestWMS(FileCopyActionExecutorTest *test,
                                         const std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    FileCopyActionExecutorTest *test;

    int main() override {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("");
        // Add a file_copy_action
        auto file_copy_action = job->addFileCopyAction("", this->test->file,
                                                       this->test->ss1,
                                                       this->test->ss2);

        // coverage
        wrench::Action::getActionTypeAsString(file_copy_action);
        file_copy_action->getFile();
        file_copy_action->getSourceFileLocation();
        file_copy_action->getDestinationFileLocation();

        // Create a file copy action executor
        auto file_copy_action_executor = std::make_shared<wrench::ActionExecutor>(
                "Host2", 0, 0.0, 0, false,
                this->commport, nullptr, file_copy_action, nullptr);
        // Start it
        file_copy_action_executor->setSimulation(this->getSimulation());
        file_copy_action_executor->start(file_copy_action_executor, true, false);

        // Wait for a message from it
        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = this->commport->getMessage();
        } catch (wrench::ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::NetworkError>(e.getCause());
            throw std::runtime_error("Network error while getting reply from Executor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg = std::dynamic_pointer_cast<wrench::ActionExecutorDoneMessage>(message);
        if (!msg) {
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        // Did the copy happen?
        if (not wrench::StorageService::hasFileAtLocation(wrench::FileLocation::LOCATION(this->test->ss2, this->test->file))) {
            throw std::runtime_error("File copy should have happened!");
        }

        // Is the start-date sensible?
        if (file_copy_action->getStartDate() < 0.0 or file_copy_action->getStartDate() > EPSILON) {
            throw std::runtime_error("Unexpected action start date: " + std::to_string(file_copy_action->getStartDate()));
        }

        // Is the end-date sensible?
        if (file_copy_action->getEndDate() < 10.8 or file_copy_action->getEndDate() > 10.9) {
            throw std::runtime_error("Unexpected action end date: " + std::to_string(file_copy_action->getEndDate()) + " (expected: ~10.9)");
        }

        // Is the state sensible?
        if (file_copy_action->getState() != wrench::Action::State::COMPLETED) {
            throw std::runtime_error("Unexpected action state: " + file_copy_action->getStateAsString());
        }

        return 0;
    }
};

TEST_F(FileCopyActionExecutorTest, Success) {
    DO_TEST_WITH_FORK(do_FileCopyActionExecutorSuccessTest_test);
}


void FileCopyActionExecutorTest::do_FileCopyActionExecutorSuccessTest_test() {

    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //        argv[1] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a Storage Service
    this->ss1 = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
            "Host3", {"/"},
            {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "1MB"}}));

    // Create another Storage Service
    this->ss2 = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
            "Host1", {"/"},
            {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "1MB"}}));

    // Create a workflow
    workflow = wrench::Workflow::createWorkflow();

    // Create a file
    this->file = wrench::Simulation::addFile("some_file", 1000000);

    // Put it on ss1
    this->ss1->createFile(wrench::FileLocation::LOCATION(this->ss1, this->file));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    wms = simulation->add(new FileCopyActionExecutorSuccessTestWMS(this, "Host1"));

    ASSERT_NO_THROW(simulation->launch());

    workflow->clear();
    wrench::Simulation::removeAllFiles();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  DO FILE_READ ACTION EXECUTOR SUCCESS TEST SAME HOST             **/
/**********************************************************************/

class FileCopyActionExecutorSuccessSameHostTestWMS : public wrench::ExecutionController {

public:
    FileCopyActionExecutorSuccessSameHostTestWMS(FileCopyActionExecutorTest *test,
                                                 const std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    FileCopyActionExecutorTest *test;

    int main() override {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("");
        // Add a file_copy_action
        auto file_copy_action = job->addFileCopyAction("",
                                                       wrench::FileLocation::LOCATION(this->test->ss1, "/", this->test->file),
                                                       wrench::FileLocation::LOCATION(this->test->ss1, "/disk2", this->test->file));

        // Create a file copy action executor
        auto file_copy_action_executor = std::shared_ptr<wrench::ActionExecutor>(
                new wrench::ActionExecutor("Host2", 0, 0.0, 0, false, this->commport, nullptr, file_copy_action, nullptr));
        // Start it
        file_copy_action_executor->setSimulation(this->getSimulation());
        file_copy_action_executor->start(file_copy_action_executor, true, false);

        // Wait for a message from it
        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = this->commport->getMessage();
        } catch (wrench::ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::NetworkError>(e.getCause());
            throw std::runtime_error("Network error while getting reply from Executor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg = std::dynamic_pointer_cast<wrench::ActionExecutorDoneMessage>(message);
        if (!msg) {
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        return 0;
    }
};

TEST_F(FileCopyActionExecutorTest, SuccessSameHost) {
    DO_TEST_WITH_FORK(do_FileCopyActionExecutorSuccessSameHostTest_test);
}


void FileCopyActionExecutorTest::do_FileCopyActionExecutorSuccessSameHostTest_test() {

    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a Storage Service
    this->ss1 = simulation->add(wrench::SimpleStorageService::createSimpleStorageService("Host3", {"/"}));

    // Create another Storage Service
    this->ss2 = simulation->add(wrench::SimpleStorageService::createSimpleStorageService("Host1", {"/"}));

    // Create a workflow
    workflow = wrench::Workflow::createWorkflow();

    // Create a file
    this->file = wrench::Simulation::addFile("some_file", 1000000000ULL);

    // Put it on ss1
    this->ss1->createFile(wrench::FileLocation::LOCATION(this->ss1, this->file));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    wms = simulation->add(new FileCopyActionExecutorSuccessSameHostTestWMS(this, "Host1"));

    ASSERT_NO_THROW(simulation->launch());

    workflow->clear();
    wrench::Simulation::removeAllFiles();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}