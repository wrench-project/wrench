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
#include <wrench/services/helpers/ServiceFailureDetectorMessage.h>

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"
#include "HostKiller.h"
#include "wrench/services/helpers/ServiceFailureDetector.h"
#include "Victim.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simulated_failures_test, "Log category for SimulatedFailuresTests");


class SimulatedFailuresTest : public ::testing::Test {

public:
    wrench::Workflow *workflow;
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr;

//    wrench::WorkflowFile *input_file;
//    wrench::WorkflowFile *output_file1;
//    wrench::WorkflowFile *output_file2;
//    wrench::WorkflowFile *output_file3;

//    wrench::WorkflowFile *output_file4;
//    wrench::WorkflowTask *task1;
//    wrench::WorkflowTask *task2;
//    wrench::WorkflowTask *task3;
//    wrench::WorkflowTask *task4;
//    wrench::WorkflowTask *task5;
//    wrench::WorkflowTask *task6;
//    wrench::ComputeService *compute_service = nullptr;
//    wrench::StorageService *storage_service = nullptr;

    void do_FailureDetectorTest_test();

protected:

    SimulatedFailuresTest() {
        // Create the simplest workflow
        workflow_unique_ptr = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
        workflow = workflow_unique_ptr.get();

//        // up from 0 to 100, down from 100 to 200, up from 200 to 300, etc.
//        std::string trace_file_content = "PERIODICITY 100\n"
//                                         " 0 1\n"
//                                         " 100 0";
//
//        std::string trace_file_name = "host.trace";
//        std::string trace_file_path = "/tmp/"+trace_file_name;
//
//        FILE *trace_file = fopen(trace_file_path.c_str(), "w");
//        fprintf(trace_file, "%s", trace_file_content.c_str());
//        fclose(trace_file);

        // Create the files
//        input_file = workflow->addFile("input_file", 10.0);
//        output_file1 = workflow->addFile("output_file1", 10.0);
//        output_file2 = workflow->addFile("output_file2", 10.0);
//        output_file3 = workflow->addFile("output_file3", 10.0);
//        output_file4 = workflow->addFile("output_file4", 10.0);
//
//        // Create the tasks
//        task1 = workflow->addTask("task_1_10s_1core", 10.0, 1, 1, 1.0, 0);
//        task2 = workflow->addTask("task_2_10s_1core", 10.0, 1, 1, 1.0, 0);
//        task3 = workflow->addTask("task_3_10s_2cores", 10.0, 2, 2, 1.0, 0);
//        task4 = workflow->addTask("task_4_10s_2cores", 10.0, 2, 2, 1.0, 0);
//        task5 = workflow->addTask("task_5_30s_1_to_3_cores", 30.0, 1, 3, 1.0, 0);
//        task6 = workflow->addTask("task_6_10s_1_to_2_cores", 12.0, 1, 2, 1.0, 0);
//        task1->setClusterID("ID1");
//        task2->setClusterID("ID1");
//        task3->setClusterID("ID1");
//        task4->setClusterID("ID2");
//        task5->setClusterID("ID2");
//
//        // Add file-task dependencies
//        task1->addInputFile(input_file);
//        task2->addInputFile(input_file);
//        task3->addInputFile(input_file);
//        task4->addInputFile(input_file);
//        task5->addInputFile(input_file);
//        task6->addInputFile(input_file);
//
//        task1->addOutputFile(output_file1);
//        task2->addOutputFile(output_file2);
//        task3->addOutputFile(output_file3);
//        task4->addOutputFile(output_file4);
//        task5->addOutputFile(output_file3);
//        task6->addOutputFile(output_file4);
//
//        workflow->addControlDependency(task4, task5);

        // Create a platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"FailedHost\" speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"StableHost\" speed=\"1f\" core=\"1\"/> "
                          "       <link id=\"link1\" bandwidth=\"100kBps\" latency=\"0\"/>"
                          "       <route src=\"FailedHost\" dst=\"StableHost\">"
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
/**                    HOST FAILURE DURING A SLEEP                   **/
/**********************************************************************/

class HostFailureDuringSleepWMS : public wrench::WMS {

public:
    HostFailureDuringSleepWMS(SimulatedFailuresTest *test,
                                      std::string &hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    SimulatedFailuresTest *test;

    int main() override {

        /** TEST THAT SHOULD DETECT A FAILURE **/

        // Starting a victim (that will reply with a bogus TTL Expiration message)
        auto victim = std::shared_ptr<wrench::Victim>(new wrench::Victim("FailedHost", 100, new wrench::ServiceTTLExpiredMessage(1), this->mailbox_name));
        victim->simulation = this->simulation;
        victim->start(victim, true);

        // Starting its nemesis!
        auto murderer = std::shared_ptr<wrench::HostKiller>(new wrench::HostKiller("StableHost", 50, "FailedHost"));
        murderer->simulation = this->simulation;
        murderer->start(murderer, true);

        // Starting the failure detector!
        auto failure_detector = std::shared_ptr<wrench::ServiceFailureDetector>(new wrench::ServiceFailureDetector("StableHost", victim.get(), this->mailbox_name));
        failure_detector->simulation = this->simulation;
        failure_detector->start(failure_detector, true);

        // Waiting for a message
        std::unique_ptr<wrench::SimulationMessage> message;
        try {
            message = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting a message!" + cause->toString());
        }

        if (dynamic_cast<wrench::ServiceTTLExpiredMessage *>(message.get())) {
            throw std::runtime_error("Failure should have been detected!");
        } else if (dynamic_cast<wrench::ServiceHasFailedMessage *>(message.get())) {
            // All good
        } else {
            throw std::runtime_error("Unexpected " + message->getName() + " message");
        }

        // Wait for the host killer to finish, and turn the host back on
        murderer->join();
        simgrid::s4u::Host::by_name("FailedHost")->turn_on();

        /** TEST THAT SHOULD NOT DETECT A FAILURE **/

        // Starting a victim (that will reply with a bogus TTL Expiration message)
        victim = std::shared_ptr<wrench::Victim>(new wrench::Victim("FailedHost", 100, new wrench::ServiceTTLExpiredMessage(1), this->mailbox_name));
        victim->simulation = this->simulation;
        victim->start(victim, true);

        // Starting its nemesis!
        murderer = std::shared_ptr<wrench::HostKiller>(new wrench::HostKiller("StableHost", 101, "FailedHost"));
        murderer->simulation = this->simulation;
        murderer->start(murderer, true);

        // Starting the failure detector!
        failure_detector = std::shared_ptr<wrench::ServiceFailureDetector>(new wrench::ServiceFailureDetector("StableHost", victim.get(), this->mailbox_name));
        failure_detector->simulation = this->simulation;
        failure_detector->start(failure_detector, true);

        // Waiting for a message
        try {
            message = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting a message!" + cause->toString());
        }

        if (dynamic_cast<wrench::ServiceTTLExpiredMessage *>(message.get())) {
            // All good
        } else if (dynamic_cast<wrench::ServiceHasFailedMessage *>(message.get())) {
            throw std::runtime_error("Failure should not have been detected!");
        } else {
            throw std::runtime_error("Unexpected " + message->getName() + " message");
        }

        return 0;
    }
};

TEST_F(SimulatedFailuresTest, FailureDetectorTest) {
    DO_TEST_WITH_FORK(do_FailureDetectorTest_test);
}

void SimulatedFailuresTest::do_FailureDetectorTest_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("failure_test");


    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "StableHost";

    // Create a WMS
    wrench::WMS *wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new HostFailureDuringSleepWMS(this, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}


