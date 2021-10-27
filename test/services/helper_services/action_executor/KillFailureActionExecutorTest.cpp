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
#include <wrench/action/ComputeAction.h>
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

WRENCH_LOG_CATEGORY(generic_action_executor_test, "Log category for KillFailActionExecutorTest");

#define EPSILON (std::numeric_limits<double>::epsilon())

class KillFailActionExecutorTest : public ::testing::Test {

public:
    wrench::Simulation *simulation;
    wrench::WorkflowFile *file;
    std::shared_ptr<wrench::StorageService> ss;

    void loopThroughTestCases(std::vector<double> points_of_interest, bool kill, std::string action_type) {
        std::vector<double> deltas = {0.000000000, 0.000000001, 0.00000001, 0.0000001, 0.000001, 0.00001, 0.0001, 0.001, 0.01, 0.1};
        for (const auto &p : points_of_interest) {
            for (const auto &d : deltas) {
                if (p - d >= 0) {
                    DO_TEST_WITH_FORK_THREE_ARGS(do_ActionExecutorKillFailTest_test, p - d, kill, action_type);
                }
            }
            for (const auto &d : deltas) {
                DO_TEST_WITH_FORK_THREE_ARGS(do_ActionExecutorKillFailTest_test, p + d, kill, action_type);
            }
        }
    }

    void do_ActionExecutorKillFailTest_test(double sleep_time, bool kill, std::string task_type);

protected:
    KillFailActionExecutorTest() {

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

};


/**********************************************************************/
/**  DO KILL FAIL ACTION EXECUTOR SUCCESS TEST                       **/
/**********************************************************************/


class KillFailActionExecutorTestWMS : public wrench::WMS {

public:
    KillFailActionExecutorTestWMS(KillFailActionExecutorTest *test,
                                  std::string hostname,
                                  double sleep_time,
                                  bool kill,
                                  std::string action_type) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
        this->sleep_time = sleep_time;
        this->kill =kill;
        this->action_type = std::move(action_type);
    }


private:

    KillFailActionExecutorTest *test;
    double thread_startup_overhead;
    double sleep_time;
    bool kill;
    std::string action_type;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("");

        // Create an action executor
        std::shared_ptr<wrench::Action> action;
        unsigned long num_cores = 0;
        double ram = 0;
        double expected_completion_date = -1;
        double thread_overhead = 0;
        if (this->action_type == "sleep") {
            action = std::dynamic_pointer_cast<wrench::Action>(job->addSleepAction("", 10.0));
            expected_completion_date = 10.2;
            thread_overhead = 0.2;
            num_cores = 0;
            ram = 0.0;
        } else if (this->action_type == "compute") {
            action = std::dynamic_pointer_cast<wrench::Action>(job->addComputeAction("", 20.0, 100.0, 1, 4, wrench::ParallelModel::AMDAHL(1.0)));
            thread_overhead = 0.1;
            expected_completion_date = 10.2;
            num_cores = 2;
            ram = 200;
        } else if (this->action_type == "file_read") {
            action = std::dynamic_pointer_cast<wrench::Action>(job->addFileReadAction("", std::shared_ptr<wrench::WorkflowFile>(this->test->file),wrench::FileLocation::LOCATION(this->test->ss)));
            thread_overhead = 0.1;
            expected_completion_date = 10.84743174020618639020;
            num_cores = 0;
            ram = 0.0;
        }

        action->setThreadCreationOverhead(thread_overhead);

        auto action_executor = std::shared_ptr<wrench::ActionExecutor>(
                new wrench::ActionExecutor("Host2",
                                           num_cores,
                                           ram,
                                           this->mailbox_name,
                                           action));

        // Start it
        action_executor->simulation = this->simulation;
        action_executor->start(action_executor, true, false);

//        WRENCH_INFO("SLEEPING %lf", this->sleep_time);
        wrench::Simulation::sleep(this->sleep_time);

        if (this->kill) {
            // Kill it
            action_executor->kill(true);
        } else {
            // Fail it
            simgrid::s4u::Host::by_name("Host2")->turn_off();
        }


        // Is the start date sensible?
        if (action->getStartDate() < 0.0 || action->getStartDate() > EPSILON) {
            throw std::runtime_error("Unexpected action start date: " + std::to_string(action->getStartDate()));
        }

        // Is the state and end date sensible?
        if ((this->sleep_time + EPSILON < expected_completion_date and
             ((this->kill and action->getState() != wrench::Action::State::KILLED) or
              ((not this->kill) and action->getState() != wrench::Action::State::FAILED))) or
            (this->sleep_time > expected_completion_date  + EPSILON and action->getState() != wrench::Action::State::COMPLETED)) {
            throw std::runtime_error("Unexpected action state : " + action->getStateAsString());
        }

        return 0;
    }
};

TEST_F(KillFailActionExecutorTest, KillSleep) {
    loopThroughTestCases({0.0, 0.2, 10.2}, true, "sleep");
}

TEST_F(KillFailActionExecutorTest, FailSleep) {
    loopThroughTestCases({0.0, 0.2, 10.2}, false, "sleep");
}

TEST_F(KillFailActionExecutorTest, KillCompute) {
    loopThroughTestCases({0.0, 0.2, 10.2}, true, "compute");
}

TEST_F(KillFailActionExecutorTest, FailCompute) {
    loopThroughTestCases({0.0, 0.2, 10.2}, false, "compute");
}

TEST_F(KillFailActionExecutorTest, KillFileRead) {
    loopThroughTestCases({0.0, 0.1, 10.14743174020618639020}, true, "file_read");
}

TEST_F(KillFailActionExecutorTest, FailFileRead) {
    loopThroughTestCases({0.0, 0.1, 10.14743174020618639020}, false, "file_read");
}

void KillFailActionExecutorTest::do_ActionExecutorKillFailTest_test(double sleep_time, bool kill, std::string action_type) {

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

    this->workflow = std::make_unique<wrench::Workflow>();

    // Create a Storage Service
    this->ss = simulation->add(new wrench::SimpleStorageService("Host3", {"/"}));

    // Create a file
    this->file = this->workflow->addFile("some_file", 1000000.0);

    ss->createFile(file, wrench::FileLocation::LOCATION(ss));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new KillFailActionExecutorTestWMS(this, "Host1", sleep_time, kill, action_type)));

    ASSERT_NO_THROW(wms->addWorkflow(this->workflow.get()));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);

}
