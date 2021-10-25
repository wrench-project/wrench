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

#include <wrench/action/ComputeAction.h>
#include <wrench/services/helper_services/action_executor/ComputeActionExecutor.h>
#include <wrench/services/helper_services/action_executor/ActionExecutorMessage.h>
#include <wrench/job/CompoundJob.h>
#include <wrench/failure_causes//HostError.h>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(compute_action_executor_test, "Log category for ComputeActionExecutorTest");

#define EPSILON (std::numeric_limits<double>::epsilon())

class ComputeActionExecutorTest : public ::testing::Test {

public:
    wrench::Simulation *simulation;

    void do_ComputeActionExecutorSuccessTest_test();
    void do_ComputeActionExecutorKillTest_test(double sleep_before_kill);
    void do_ComputeActionExecutorFailureTest_test(double sleep_before_kill);


protected:
    ComputeActionExecutorTest() {

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
/**  DO COMPUTE ACTION EXECUTOR SUCCESS TEST                         **/
/**********************************************************************/


class ComputeActionExecutorSuccessTestWMS : public wrench::WMS {

public:
    ComputeActionExecutorSuccessTestWMS(ComputeActionExecutorTest *test,
                                        std::string hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }


private:

    ComputeActionExecutorTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("");
        // Add a compute action
        auto compute_action = job->addComputeAction("", 20.0, 100.0, 1, 4, wrench::ParallelModel::AMDAHL(1.0));
        // Create a compute action executor
        double thread_startup_overhead = 0.1;
        unsigned long num_cores = 2;
        auto compute_action_executor = std::shared_ptr<wrench::ComputeActionExecutor>(
                new wrench::ComputeActionExecutor("Host2",
                                                  num_cores,
                                                  200.0,
                                                  this->mailbox_name,
                                                  compute_action,
                                                  thread_startup_overhead,
                                                  false));

        // Start it
        compute_action_executor->simulation = this->simulation;
        compute_action_executor->start(compute_action_executor, true, false);
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
        if (compute_action->getStartDate() < 0.0 or compute_action->getStartDate() > EPSILON) {
            throw std::runtime_error("Unexpected action start date: " + std::to_string(compute_action->getEndDate()));
        }

        // Is the end-date sensible?
        if (compute_action->getEndDate() < 10.0 or compute_action->getEndDate() > 10.0 + num_cores * thread_startup_overhead + EPSILON) {
            throw std::runtime_error("Unexpected action end date: " + std::to_string(compute_action->getEndDate()));
        }

        // Is the state sensible?
        if (compute_action->getState() != wrench::Action::State::COMPLETED) {
            throw std::runtime_error("Unexpected action state: " + compute_action->getStateAsString());
        }

        return 0;
    }
};

TEST_F(ComputeActionExecutorTest, SuccessTest) {
    DO_TEST_WITH_FORK(do_ComputeActionExecutorSuccessTest_test);
}


void ComputeActionExecutorTest::do_ComputeActionExecutorSuccessTest_test() {

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
            new ComputeActionExecutorSuccessTestWMS(this, "Host1")));

    ASSERT_NO_THROW(wms->addWorkflow(new wrench::Workflow()));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);

}

/**********************************************************************/
/**  DO COMPUTE ACTION EXECUTOR KILL TEST                            **/
/**********************************************************************/


class ComputeActionExecutorKillTestWMS : public wrench::WMS {

public:
    ComputeActionExecutorKillTestWMS(ComputeActionExecutorTest *test,
                                     std::string hostname,
                                     double sleep_before_kill) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
        this->sleep_before_kill = sleep_before_kill;
    }


private:

    ComputeActionExecutorTest *test;
    double sleep_before_kill;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("");
        // Add a compute action
        auto compute_action = job->addComputeAction("", 20.0, 100.0, 1, 4, wrench::ParallelModel::AMDAHL(1.0));
        // Create a compute action executor
        double thread_startup_overhead = 0.1;
        unsigned long num_cores = 2;
        auto compute_action_executor = std::shared_ptr<wrench::ComputeActionExecutor>(
                new wrench::ComputeActionExecutor("Host2",
                                                  num_cores,
                                                  200.0,
                                                  this->mailbox_name,
                                                  compute_action,
                                                  thread_startup_overhead,
                                                  false));

        // Start it
        compute_action_executor->simulation = this->simulation;
        compute_action_executor->start(compute_action_executor, true, false);

        // Sleep
        wrench::Simulation::sleep(this->sleep_before_kill);

        // Kill it
        compute_action_executor->kill(true);

        // Is the start date sensible?
        if (compute_action->getStartDate() < 0.0 || compute_action->getStartDate() > EPSILON) {
            throw std::runtime_error("Unexpected action start date: " + std::to_string(compute_action->getStartDate()));
        }

        // Is the state and end date sensible?
        if ((this->sleep_before_kill + EPSILON < 10.2 and compute_action->getState() != wrench::Action::State::KILLED) or
            (this->sleep_before_kill > 10.2  + EPSILON and compute_action->getState() != wrench::Action::State::COMPLETED)) {
            throw std::runtime_error("Unexpected action state : " + compute_action->getStateAsString());
        }

        return 0;
    }
};

TEST_F(ComputeActionExecutorTest, KillTest) {
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.0);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.0000001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.000001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.00001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.0001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.01);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.9);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.99);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.9999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.99999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.999999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.9999999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.99999999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.1);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.1000001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.100001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.10001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.1001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.101);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.11);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.19);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.199);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.1999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.19999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.199999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.1999999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.2);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.2000001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.200001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.20001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.2001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.201);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 0.21);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 5.0);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 10.19);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 10.199);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 10.1999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 10.19999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 10.199999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 10.1999999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 10.19999999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorKillTest_test, 10.2);
}


void ComputeActionExecutorTest::do_ComputeActionExecutorKillTest_test(double sleep_before_kill) {

    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
//    argv[1] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new ComputeActionExecutorKillTestWMS(this, "Host1", sleep_before_kill)));

    ASSERT_NO_THROW(wms->addWorkflow(new wrench::Workflow()));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);

}

/**********************************************************************/
/**  DO COMPUTE ACTION EXECUTOR FAILURE TEST                         **/
/**********************************************************************/


class ComputeActionExecutorFailureTestWMS : public wrench::WMS {

public:
    ComputeActionExecutorFailureTestWMS(ComputeActionExecutorTest *test,
                                     std::string hostname,
                                     double sleep_before_fail) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
        this->sleep_before_fail = sleep_before_fail;
    }


private:

    ComputeActionExecutorTest *test;
    double sleep_before_fail;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("");
        // Add a compute action
        auto compute_action = job->addComputeAction("", 20.0, 100.0, 1, 4, wrench::ParallelModel::AMDAHL(1.0));
        // Create a compute action executor
        double thread_startup_overhead = 0.1;
        unsigned long num_cores = 2;
        auto compute_action_executor = std::shared_ptr<wrench::ComputeActionExecutor>(
                new wrench::ComputeActionExecutor("Host2",
                                                  num_cores,
                                                  200.0,
                                                  this->mailbox_name,
                                                  compute_action,
                                                  thread_startup_overhead,
                                                  false));

        // Start it
        compute_action_executor->simulation = this->simulation;
        compute_action_executor->start(compute_action_executor, true, false);

        // Sleep
        wrench::Simulation::sleep(this->sleep_before_fail);

        // Fail it
        simgrid::s4u::Host::by_name("Host2")->turn_off();

        // Is the start date sensible?
        if (compute_action->getStartDate() < 0.0 || compute_action->getStartDate() > EPSILON) {
            throw std::runtime_error("Unexpected action start date: " + std::to_string(compute_action->getStartDate()));
        }

        // Is the state and end date sensible?
        if ((this->sleep_before_fail + EPSILON < 10.2 and compute_action->getState() != wrench::Action::State::FAILED) or
            (this->sleep_before_fail > 10.2  + EPSILON and compute_action->getState() != wrench::Action::State::COMPLETED)) {
            throw std::runtime_error("Unexpected action state : " + compute_action->getStateAsString());
        }

        // If the failure cause set?
        if (compute_action->getState() == wrench::Action::State::FAILED) {
            if (not compute_action->getFailureCause()) {
                throw std::runtime_error("Missing failure cause");
            } else if (not std::dynamic_pointer_cast<wrench::HostError>(compute_action->getFailureCause())) {
                throw std::runtime_error("Unexpected failure cause: " + compute_action->getFailureCause()->toString());
            }
        }

        return 0;
    }
};

TEST_F(ComputeActionExecutorTest, FailureTest) {
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.0);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.0000001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.000001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.00001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.0001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.01);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.9);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.99);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.9999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.99999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.999999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.9999999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.99999999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.1);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.1000001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.100001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.10001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.1001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.101);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.11);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.19);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.199);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.1999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.19999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.199999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.1999999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.2);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.2000001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.200001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.20001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.2001);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.201);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 0.21);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 5.0);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 10.19);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 10.199);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 10.1999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 10.19999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 10.199999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 10.1999999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 10.19999999);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorFailureTest_test, 10.2);
}


void ComputeActionExecutorTest::do_ComputeActionExecutorFailureTest_test(double sleep_before_fail) {

    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 2;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");
//    argv[2] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new ComputeActionExecutorFailureTestWMS(this, "Host1", sleep_before_fail)));

    ASSERT_NO_THROW(wms->addWorkflow(new wrench::Workflow()));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);

}

