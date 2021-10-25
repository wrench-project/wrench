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

#include <wrench/action/FileReadAction.h>
#include <wrench/services/helper_services/action_executor/FileReadActionExecutor.h>
#include <wrench/services/helper_services/action_executor/ActionExecutorMessage.h>
#include <wrench/job/CompoundJob.h>
#include <wrench/failure_causes//HostError.h>

#include <memory>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(file_read_action_executor_test, "Log category for FileReadActionExecutorTest");

#define EPSILON (std::numeric_limits<double>::epsilon())

class FileReadActionExecutorTest : public ::testing::Test {

public:
    wrench::Simulation *simulation;

    void do_FileReadActionExecutorSuccessTest_test();
    void do_FileReadActionExecutorKillTest_test(double sleep_before_kill);
    void do_FileReadActionExecutorFailureTest_test(double sleep_before_fail);


protected:
    FileReadActionExecutorTest() {

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
    std::unique_ptr<wrench::Workflow> workflow;

public:
    wrench::WorkflowFile *file;
    std::shared_ptr<wrench::StorageService> ss;

};


/**********************************************************************/
/**  DO FILE_READ ACTION EXECUTOR SUCCESS TEST                       **/
/**********************************************************************/


class FileReadActionExecutorSuccessTestWMS : public wrench::WMS {

public:
    FileReadActionExecutorSuccessTestWMS(FileReadActionExecutorTest *test,
                                         std::string hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }


private:

    FileReadActionExecutorTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("");
        // Add a file_read_action
        auto file_read_action = job->addFileReadAction("", std::shared_ptr<wrench::WorkflowFile>(this->test->file),
                                                       wrench::FileLocation::LOCATION(this->test->ss));
        // Create a sleep action executor
        auto file_read_action_executor = std::shared_ptr<wrench::FileReadActionExecutor>(
                new wrench::FileReadActionExecutor("Host2", this->mailbox_name, file_read_action));
        // Start it
        file_read_action_executor->simulation = this->simulation;
        file_read_action_executor->start(file_read_action_executor, true, false);
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
        if (file_read_action->getStartDate() < 0.0 or file_read_action->getStartDate() > EPSILON) {
            throw std::runtime_error("Unexpected action start date: " + std::to_string(file_read_action->getEndDate()));
        }

        // Is the end-date sensible?
        if (file_read_action->getEndDate() + EPSILON < 10.0 or file_read_action->getEndDate() > 11.0 + EPSILON) {
            throw std::runtime_error("Unexpected action end date: " + std::to_string(file_read_action->getEndDate()));
        }

        // Is the state sensible?
        if (file_read_action->getState() != wrench::Action::State::COMPLETED) {
            throw std::runtime_error("Unexpected action state: " + file_read_action->getStateAsString());
        }

        return 0;
    }
};

TEST_F(FileReadActionExecutorTest, SuccessTest) {
    DO_TEST_WITH_FORK(do_FileReadActionExecutorSuccessTest_test);
}


void FileReadActionExecutorTest::do_FileReadActionExecutorSuccessTest_test() {

    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a Storage Service
    this->ss = simulation->add(new wrench::SimpleStorageService("Host3", {"/"}));

    // Create a workflow
    workflow = std::make_unique<wrench::Workflow>();

    // Create a file
    this->file = workflow->addFile("some_file", 1000000.0);

    ss->createFile(file, wrench::FileLocation::LOCATION(ss));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    wms = simulation->add(new FileReadActionExecutorSuccessTestWMS(this, "Host1"));

    wms->addWorkflow(workflow.get());

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);

}


#if 0

/**********************************************************************/
/**  DO FILE_READ ACTION EXECUTOR KILL TEST                          **/
/**********************************************************************/


class FileReadActionExecutorKillTestWMS : public wrench::WMS {

public:
    FileReadActionExecutorKillTestWMS(FileReadActionExecutorTest *test,
                                   std::string hostname,
                                   double sleep_before_kill) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
        this->sleep_before_kill = sleep_before_kill;
    }


private:

    FileReadActionExecutorTest *test;
    double sleep_before_kill;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("");
        // Add a file_read_action
        auto file_read_action = job->addFileReadAction("", 10.0);
        // Create a sleep action executor
        auto file_read_action_executor = std::shared_ptr<wrench::FileReadActionExecutor>(
                new wrench::FileReadActionExecutor("Host2", this->mailbox_name, file_read_action));
        // Start it
        file_read_action_executor->simulation = this->simulation;
        file_read_action_executor->start(file_read_action_executor, true, false);

        // Sleep
        wrench::Simulation::sleep(this->sleep_before_kill);

        // Kill it
        file_read_action_executor->kill(true);

        // Is the start date sensible?
        if (file_read_action->getStartDate() < 0.0 || file_read_action->getStartDate() > EPSILON) {
            throw std::runtime_error("Unexpected action start date: " + std::to_string(file_read_action->getStartDate()));
        }

        // Is the end date sensible?
        if (file_read_action->getEndDate() + EPSILON < this->sleep_before_kill || file_read_action->getEndDate() > this->sleep_before_kill + EPSILON) {
            throw std::runtime_error("Unexpected action end date: " + std::to_string(file_read_action->getEndDate()));
        }

        // Is the state sensible?
        if ((this->sleep_before_kill  + EPSILON < 10.0 and file_read_action->getState() != wrench::Action::State::KILLED) or
            (this->sleep_before_kill > 10.0 + EPSILON  and file_read_action->getState() != wrench::Action::State::COMPLETED)) {
            throw std::runtime_error("Unexpected action state: " + file_read_action->getStateAsString());
        }

        return 0;
    }
};

TEST_F(FileReadActionExecutorTest, KillTest) {
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorKillTest_test, 0.0);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorKillTest_test, 0.000001);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorKillTest_test, 0.00001);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorKillTest_test, 0.0001);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorKillTest_test, 0.001);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorKillTest_test, 0.01);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorKillTest_test, 0.1);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorKillTest_test, 5.0);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorKillTest_test, 9.90);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorKillTest_test, 9.99);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorKillTest_test, 9.999);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorKillTest_test, 9.9999);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorKillTest_test, 9.99999);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorKillTest_test, 9.999999);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorKillTest_test, 10.000);
}


void FileReadActionExecutorTest::do_FileReadActionExecutorKillTest_test(double sleep_before_kill) {

    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new FileReadActionExecutorKillTestWMS(this, "Host1", sleep_before_kill)));

    ASSERT_NO_THROW(wms->addWorkflow(new wrench::Workflow()));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);

}




/**********************************************************************/
/**  DO FILE_READ ACTION EXECUTOR FAILURE TEST                           **/
/**********************************************************************/


class FileReadActionExecutorFailureTestWMS : public wrench::WMS {

public:
    FileReadActionExecutorFailureTestWMS(FileReadActionExecutorTest *test,
                                      std::string hostname, double sleep_before_fail) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
        this->sleep_before_fail = sleep_before_fail;
    }


private:

    FileReadActionExecutorTest *test;
    double sleep_before_fail;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("");
        // Add a file_read_action
        auto file_read_action = job->addFileReadAction("", 10.0);
        // Create a sleep action executor
        auto file_read_action_executor = std::shared_ptr<wrench::FileReadActionExecutor>(
                new wrench::FileReadActionExecutor("Host2", this->mailbox_name, file_read_action));
        // Start it
        file_read_action_executor->simulation = this->simulation;
        file_read_action_executor->start(file_read_action_executor, true, false);

        // Sleep
        wrench::Simulation::sleep(this->sleep_before_fail);

        // Turn off the hosts
        simgrid::s4u::Host::by_name("Host2")->turn_off();

        // Is the start date sensible?
        if (file_read_action->getStartDate() < 0.0 || file_read_action->getStartDate() > EPSILON) {
            throw std::runtime_error("Unexpected action start date: " + std::to_string(file_read_action->getStartDate()));
        }

        // Is the end date sensible?
        if (file_read_action->getEndDate() + EPSILON < this->sleep_before_fail || file_read_action->getEndDate() > this->sleep_before_fail + EPSILON) {
            throw std::runtime_error("Unexpected action end date: " + std::to_string(file_read_action->getEndDate()));
        }

        // Is the state sensible?
        if ((this->sleep_before_fail + EPSILON < 10.0  and file_read_action->getState() != wrench::Action::State::FAILED) or
            (this->sleep_before_fail > 10.0 + EPSILON and file_read_action->getState() != wrench::Action::State::COMPLETED)) {
            throw std::runtime_error("Unexpected action state: " + file_read_action->getStateAsString());
        }

        if (file_read_action->getState() == wrench::Action::State::FAILED) {
            if (not file_read_action->getFailureCause()) {
                throw std::runtime_error("Missing failure cause");
            } else if (not std::dynamic_pointer_cast<wrench::HostError>(file_read_action->getFailureCause())) {
                throw std::runtime_error("Unexpected failure cause: " + file_read_action->getFailureCause()->toString());
            }
        }

        return 0;
    }
};

TEST_F(FileReadActionExecutorTest, FailureTest) {
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorFailureTest_test, 0.0);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorFailureTest_test, 0.000001);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorFailureTest_test, 0.00001);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorFailureTest_test, 0.0001);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorFailureTest_test, 0.001);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorFailureTest_test, 0.01);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorFailureTest_test, 0.1);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorFailureTest_test, 5.0);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorFailureTest_test, 9.90);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorFailureTest_test, 9.99);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorFailureTest_test, 9.999);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorFailureTest_test, 9.9999);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorFailureTest_test, 9.99999);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorFailureTest_test, 9.999999);
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorFailureTest_test, 10.000);
}

void FileReadActionExecutorTest::do_FileReadActionExecutorFailureTest_test(double sleep_before_fail) {

    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 2;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new FileReadActionExecutorFailureTestWMS(this, "Host1", sleep_before_fail)));

    ASSERT_NO_THROW(wms->addWorkflow(new wrench::Workflow()));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);

}


#endif
