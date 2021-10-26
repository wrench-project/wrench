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

#include <wrench/action/SleepAction.h>
#include <wrench/services/helper_services/action_executor/ActionExecutorMessage.h>
#include <wrench/job/CompoundJob.h>
#include <wrench/failure_causes//HostError.h>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(sleep_action_executor_test, "Log category for SleepActionExecutorTest");

#define EPSILON (std::numeric_limits<double>::epsilon())

class SleepActionExecutorTest : public ::testing::Test {

public:
    wrench::Simulation *simulation;

    void do_SleepActionExecutorSuccessTest_test();
    void do_SleepActionExecutorKillTest_test(double sleep_before_kill);
    void do_SleepActionExecutorFailureTest_test(double sleep_before_fail);


protected:
    SleepActionExecutorTest() {

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

};


/**********************************************************************/
/**  DO SLEEP ACTION EXECUTOR SUCCESS TEST                           **/
/**********************************************************************/


class SleepActionExecutorSuccessTestWMS : public wrench::WMS {

public:
    SleepActionExecutorSuccessTestWMS(SleepActionExecutorTest *test,
                                      std::string hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }


private:

    SleepActionExecutorTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("");
        // Add a sleep_action
        auto sleep_action = job->addSleepAction("", 10.0);
        // Create a sleep action executor
        auto sleep_action_executor = std::shared_ptr<wrench::ActionExecutor>(
                new wrench::ActionExecutor("Host2", 0, 0.0,this->mailbox_name, sleep_action));
        // Start it
        sleep_action_executor->simulation = this->simulation;
        sleep_action_executor->start(sleep_action_executor, true, false);
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
        if (sleep_action->getStartDate() < 0.0 or sleep_action->getStartDate() > EPSILON) {
            throw std::runtime_error("Unexpected action start date: " + std::to_string(sleep_action->getEndDate()));
        }

        // Is the end-date sensible?
        if (sleep_action->getEndDate() + EPSILON < 10.0 or sleep_action->getEndDate() > 10.0 + EPSILON) {
            throw std::runtime_error("Unexpected action end date: " + std::to_string(sleep_action->getEndDate()));
        }

        // Is the state sensible?
        if (sleep_action->getState() != wrench::Action::State::COMPLETED) {
            throw std::runtime_error("Unexpected action state: " + sleep_action->getStateAsString());
        }

        return 0;
    }
};

TEST_F(SleepActionExecutorTest, SuccessTest) {
    DO_TEST_WITH_FORK(do_SleepActionExecutorSuccessTest_test);
}


void SleepActionExecutorTest::do_SleepActionExecutorSuccessTest_test() {

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
            new SleepActionExecutorSuccessTestWMS(this, "Host1")));

    ASSERT_NO_THROW(wms->addWorkflow(new wrench::Workflow()));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);

}

/**********************************************************************/
/**  DO SLEEP ACTION EXECUTOR KILL TEST                              **/
/**********************************************************************/


class SleepActionExecutorKillTestWMS : public wrench::WMS {

public:
    SleepActionExecutorKillTestWMS(SleepActionExecutorTest *test,
                                   std::string hostname,
                                   double sleep_before_kill) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
        this->sleep_before_kill = sleep_before_kill;
    }


private:

    SleepActionExecutorTest *test;
    double sleep_before_kill;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("");
        // Add a sleep_action
        auto sleep_action = job->addSleepAction("", 10.0);
        // Create a sleep action executor
        auto sleep_action_executor = std::shared_ptr<wrench::ActionExecutor>(
                new wrench::ActionExecutor("Host2", 0, 0.0, this->mailbox_name, sleep_action));
        // Start it
        sleep_action_executor->simulation = this->simulation;
        sleep_action_executor->start(sleep_action_executor, true, false);

        // Sleep
        wrench::Simulation::sleep(this->sleep_before_kill);

        // Kill it
        sleep_action_executor->kill(true);

        // Is the start date sensible?
        if (sleep_action->getStartDate() < 0.0 || sleep_action->getStartDate() > EPSILON) {
            throw std::runtime_error("Unexpected action start date: " + std::to_string(sleep_action->getStartDate()));
        }

        // Is the state and end date sensible?
        if ((this->sleep_before_kill  + EPSILON < 10.0 and sleep_action->getState() != wrench::Action::State::KILLED) or
            (this->sleep_before_kill > 10.0 + EPSILON  and sleep_action->getState() != wrench::Action::State::COMPLETED)) {
            throw std::runtime_error("Unexpected action state: " + sleep_action->getStateAsString());
        }

        return 0;
    }
};

TEST_F(SleepActionExecutorTest, KillTest) {
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorKillTest_test, 0.0);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorKillTest_test, 0.000001);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorKillTest_test, 0.00001);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorKillTest_test, 0.0001);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorKillTest_test, 0.001);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorKillTest_test, 0.01);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorKillTest_test, 0.1);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorKillTest_test, 5.0);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorKillTest_test, 9.90);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorKillTest_test, 9.99);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorKillTest_test, 9.999);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorKillTest_test, 9.9999);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorKillTest_test, 9.99999);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorKillTest_test, 9.999999);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorKillTest_test, 10.000);
}


void SleepActionExecutorTest::do_SleepActionExecutorKillTest_test(double sleep_before_kill) {

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
            new SleepActionExecutorKillTestWMS(this, "Host1", sleep_before_kill)));

    ASSERT_NO_THROW(wms->addWorkflow(new wrench::Workflow()));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);

}




/**********************************************************************/
/**  DO SLEEP ACTION EXECUTOR FAILURE TEST                           **/
/**********************************************************************/


class SleepActionExecutorFailureTestWMS : public wrench::WMS {

public:
    SleepActionExecutorFailureTestWMS(SleepActionExecutorTest *test,
                                      std::string hostname, double sleep_before_fail) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
        this->sleep_before_fail = sleep_before_fail;
    }


private:

    SleepActionExecutorTest *test;
    double sleep_before_fail;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("");
        // Add a sleep_action
        auto sleep_action = job->addSleepAction("", 10.0);
        // Create a sleep action executor
        auto sleep_action_executor = std::shared_ptr<wrench::ActionExecutor>(
                new wrench::ActionExecutor("Host2", 0, 0.0,this->mailbox_name, sleep_action));
        // Start it
        sleep_action_executor->simulation = this->simulation;
        sleep_action_executor->start(sleep_action_executor, true, false);

        // Sleep
        wrench::Simulation::sleep(this->sleep_before_fail);

        // Turn off the hosts
        simgrid::s4u::Host::by_name("Host2")->turn_off();

        // Is the start date sensible?
        if (sleep_action->getStartDate() < 0.0 || sleep_action->getStartDate() > EPSILON) {
            throw std::runtime_error("Unexpected action start date: " + std::to_string(sleep_action->getStartDate()));
        }

        // Is the state and end date sensible?
        if ((this->sleep_before_fail + EPSILON < 10.0  and sleep_action->getState() != wrench::Action::State::FAILED) or
            (this->sleep_before_fail > 10.0 + EPSILON and sleep_action->getState() != wrench::Action::State::COMPLETED)) {
            throw std::runtime_error("Unexpected action state: " + sleep_action->getStateAsString());
        }

        if (sleep_action->getState() == wrench::Action::State::FAILED) {
            if (not sleep_action->getFailureCause()) {
                throw std::runtime_error("Missing failure cause");
            } else if (not std::dynamic_pointer_cast<wrench::HostError>(sleep_action->getFailureCause())) {
                throw std::runtime_error("Unexpected failure cause: " + sleep_action->getFailureCause()->toString());
            }
        }

        return 0;
    }
};

TEST_F(SleepActionExecutorTest, FailureTest) {
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorFailureTest_test, 0.0);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorFailureTest_test, 0.000001);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorFailureTest_test, 0.00001);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorFailureTest_test, 0.0001);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorFailureTest_test, 0.001);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorFailureTest_test, 0.01);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorFailureTest_test, 0.1);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorFailureTest_test, 5.0);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorFailureTest_test, 9.90);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorFailureTest_test, 9.99);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorFailureTest_test, 9.999);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorFailureTest_test, 9.9999);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorFailureTest_test, 9.99999);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorFailureTest_test, 9.999999);
    DO_TEST_WITH_FORK_ONE_ARG(do_SleepActionExecutorFailureTest_test, 10.000);
}

void SleepActionExecutorTest::do_SleepActionExecutorFailureTest_test(double sleep_before_fail) {

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
            new SleepActionExecutorFailureTestWMS(this, "Host1", sleep_before_fail)));

    ASSERT_NO_THROW(wms->addWorkflow(new wrench::Workflow()));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);

}



