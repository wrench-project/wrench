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

#include <wrench/action/Action.h>
#include <wrench/action/SleepAction.h>
#include <wrench/action/CustomAction.h>
#include <wrench/action/FileReadAction.h>
#include <wrench/action/FileWriteAction.h>
#include <wrench/action/FileCopyAction.h>
#include <wrench/services/helper_services/action_executor/ActionExecutorMessage.h>
#include <wrench/services/helper_services/action_executor/ActionExecutor.h>
#include <wrench/job/CompoundJob.h>
#include <wrench/failure_causes/HostError.h>

#include <utility>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(custom_action_executor_test, "Log category for CustomActionExecutorTest");

#define EPSILON (std::numeric_limits<double>::epsilon())

class CustomActionExecutorTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::Simulation> simulation;
    std::shared_ptr<wrench::DataFile> file;
    std::shared_ptr<wrench::StorageService> ss;

    void do_CustomActionExecutorSuccessTest_test();

protected:
    CustomActionExecutorTest() {

        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/disk1/\"/>"
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
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
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

};


/**********************************************************************/
/**  CUSTOM ACTION EXECUTOR SUCCESS TEST                             **/
/**********************************************************************/


class CustomActionExecutorTestWMS : public wrench::WMS {

public:
    CustomActionExecutorTestWMS(CustomActionExecutorTest *test,
                                 std::string hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }


private:

    CustomActionExecutorTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("");

        // Create an action executor
        std::shared_ptr<wrench::Action> action;
        unsigned long num_cores = 0;
        double ram = 0;

        auto storage_service = this->test->ss;
        auto file = this->test->file;
        auto lambda_execute = [storage_service, file](const std::shared_ptr<wrench::ActionExecutor>& action_executor) {
            storage_service->readFile(file, wrench::FileLocation::LOCATION(storage_service));
            wrench::Simulation::sleep(10.0);
        };
        auto lambda_terminate = [](const std::shared_ptr<wrench::ActionExecutor>& action_executor) { };

        action = std::dynamic_pointer_cast<wrench::Action>(job->addCustomAction("", lambda_execute, lambda_terminate));
        action->setThreadCreationOverhead(0.0);

        auto action_executor = std::shared_ptr<wrench::ActionExecutor>(
                new wrench::ActionExecutor("Host2",
                                           num_cores,
                                           ram,
                                           this->mailbox_name,
                                           action, nullptr));

        // Start it
        action_executor->setSimulation(this->simulation);
        action_executor->start(action_executor, true, false);

        // Wait for a message from it
        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting reply from Executor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg = std::dynamic_pointer_cast<wrench::ActionExecutorDoneMessage>(message);
        if (!msg) {
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        // Is the start-date sensible?
        if (action->getStartDate() < 0.0 or action->getStartDate() > EPSILON) {
            throw std::runtime_error("Unexpected action start date: " + std::to_string(action->getEndDate()));
        }

        // Is the end-date sensible?
        if (action->getEndDate() + EPSILON < 20.84743174020618639020 or action->getEndDate() > 20.84743174020618639020 + EPSILON) {
            throw std::runtime_error("Unexpected action end date: " + std::to_string(action->getEndDate()));
        }

        // Is the state sensible?
        if (action->getState() != wrench::Action::State::COMPLETED) {
            throw std::runtime_error("Unexpected action state: " + action->getStateAsString());
        }

        return 0;
    }
};

TEST_F(CustomActionExecutorTest, Success) {
    DO_TEST_WITH_FORK(do_CustomActionExecutorSuccessTest_test);
}

void CustomActionExecutorTest::do_CustomActionExecutorSuccessTest_test() {

    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");
//    argv[2] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    this->workflow = wrench::Workflow::createWorkflow();

    // Create a Storage Service
    this->ss = simulation->add(new wrench::SimpleStorageService("Host3", {"/"}));

    // Create a file
    this->file = this->workflow->addFile("some_file", 1000000.0);

    ss->createFile(file, wrench::FileLocation::LOCATION(ss));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new CustomActionExecutorTestWMS(this, "Host1")));

    ASSERT_NO_THROW(wms->addWorkflow(this->workflow));

    ASSERT_NO_THROW(simulation->launch());

    this->workflow->clear();

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);

}
