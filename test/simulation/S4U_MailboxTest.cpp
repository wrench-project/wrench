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
#include "../include/UniqueTmpPathPrefix.h"
#include "../include/TestWithFork.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_mailbox_test, "Log category for S4U_Mailbo test");


class S4U_MailboxTest : public ::testing::Test {

public:

    std::shared_ptr<wrench::WMS> wms1, wms2;

    void do_AsynchronousCommunication_test();

protected:
    S4U_MailboxTest() {


        // Create a two-host platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"Host2\" speed=\"1f\" core=\"1\"/> "
                          "       <link id=\"1\" bandwidth=\"1Bps\" latency=\"0us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";

};

/**********************************************************************/
/**  ASYNCHRONOUS COMMUNICATION TEST                                 **/
/**********************************************************************/

class AsynchronousCommunicationTestWMS : public wrench::WMS {

public:
    AsynchronousCommunicationTestWMS(S4U_MailboxTest *test,
                                     std::string hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }


private:

    S4U_MailboxTest *test;
    std::string mode;

    int main() {

        unsigned long index;
        if (this == this->test->wms1.get()) {
            /** SENDER **/

            // Empty set of pending comms
            std::vector<wrench::S4U_PendingCommunication *> empty_pending_comms;
            try {
                wrench::S4U_PendingCommunication::waitForSomethingToHappen(empty_pending_comms, -1);
                throw std::runtime_error("Was expecting a std::invalid_argument exception");
            } catch (std::invalid_argument &) {
            }

            // One send
            auto pending_send = wrench::S4U_Mailbox::iputMessage(this->test->wms2->mailbox_name, new wrench::SimulationMessage("foo", 100));
            pending_send->wait();

            // Two sends, no timeout
            std::vector<std::shared_ptr<wrench::S4U_PendingCommunication>> sends;
            sends.push_back(wrench::S4U_Mailbox::iputMessage(this->test->wms2->mailbox_name, new wrench::SimulationMessage("foo1", 100)));
            sends.push_back(wrench::S4U_Mailbox::iputMessage(this->test->wms2->mailbox_name, new wrench::SimulationMessage("foo2", 100)));
            index = wrench::S4U_PendingCommunication::waitForSomethingToHappen(sends, -1);
            sends.at(index)->wait();
            index = wrench::S4U_PendingCommunication::waitForSomethingToHappen(sends, -1);
            sends.at(index)->wait();

            // Two sends, timeout
            std::vector<std::shared_ptr<wrench::S4U_PendingCommunication>> sends_timeout;
            sends_timeout.push_back(wrench::S4U_Mailbox::iputMessage(this->test->wms2->mailbox_name, new wrench::SimulationMessage("foo1", 100)));
            sends_timeout.push_back(wrench::S4U_Mailbox::iputMessage(this->test->wms2->mailbox_name, new wrench::SimulationMessage("foo2", 100)));
            double now = wrench::Simulation::getCurrentSimulatedDate();
            index = wrench::S4U_PendingCommunication::waitForSomethingToHappen(sends_timeout, 10);
            if (index != ULONG_MAX) {
                throw std::runtime_error("Was expecting a timeout on the waitForSomethingToHappen");
            }
            if (fabs(10 - (wrench::Simulation::getCurrentSimulatedDate() - now)) > 0.5) {
                throw std::runtime_error("Seems like we didn't wait for the timeout!");
            }
            index = wrench::S4U_PendingCommunication::waitForSomethingToHappen(sends_timeout, 1000);
            sends_timeout.at(index)->wait();
            index = wrench::S4U_PendingCommunication::waitForSomethingToHappen(sends_timeout, 1000);
            sends_timeout.at(index)->wait();

            // One send, network failure
            pending_send = wrench::S4U_Mailbox::iputMessage(this->test->wms2->mailbox_name, new wrench::SimulationMessage("foo", 100));
            wrench::Simulation::sleep(10);
            simgrid::s4u::Link::by_name("1")->turn_off();
            try {
                pending_send->wait();
                throw std::runtime_error("Should have gotten a NetworkError");
            } catch (std::shared_ptr<wrench::NetworkError> &e) {
            }
            wrench::Simulation::sleep(10);
            simgrid::s4u::Link::by_name("1")->turn_on();
            wrench::Simulation::sleep(10);

            // Two asynchronous sends, network failure
            std::vector<std::shared_ptr<wrench::S4U_PendingCommunication>> sends_failure;
            sends_failure.push_back(wrench::S4U_Mailbox::iputMessage(this->test->wms2->mailbox_name, new wrench::SimulationMessage("foo1", 100)));
            sends_failure.push_back(wrench::S4U_Mailbox::iputMessage(this->test->wms2->mailbox_name, new wrench::SimulationMessage("foo2", 100)));
            wrench::Simulation::sleep(10);
            simgrid::s4u::Link::by_name("1")->turn_off();
            index = wrench::S4U_PendingCommunication::waitForSomethingToHappen(sends_failure, 10000);
            try {
                sends_failure.at(index)->wait();
                throw std::runtime_error("Should have gotten a NetworkError");
            } catch (std::shared_ptr<wrench::NetworkError> &e) {
            }
            index = wrench::S4U_PendingCommunication::waitForSomethingToHappen(sends_failure, 10000);
            try {
                sends_failure.at(index)->wait();
                throw std::runtime_error("Should have gotten a NetworkError");
            } catch (std::shared_ptr<wrench::NetworkError> &e) {
            }

            // One synchronous sends, network failure
            try {
                wrench::S4U_Mailbox::putMessage(this->test->wms2->mailbox_name,
                                                new wrench::SimulationMessage("foo", 100));
                throw std::runtime_error("Should have gotten a NetworkError");
            } catch (std::shared_ptr<wrench::NetworkError> &e) {
            }



        } else {
            /** RECEIVER **/

            // One recv
            auto pending_recv = wrench::S4U_Mailbox::igetMessage(this->test->wms2->mailbox_name);
            pending_recv->wait();

            // Two recv, no timeout
            std::vector<std::shared_ptr<wrench::S4U_PendingCommunication>> recvs;
            recvs.push_back(wrench::S4U_Mailbox::igetMessage(this->test->wms2->mailbox_name));
            recvs.push_back(wrench::S4U_Mailbox::igetMessage(this->test->wms2->mailbox_name));
            index = wrench::S4U_PendingCommunication::waitForSomethingToHappen(recvs, -1);
            recvs.at(index)->wait();
            index = wrench::S4U_PendingCommunication::waitForSomethingToHappen(recvs, -1);
            recvs.at(index)->wait();

            // Two recvs (sends are timing out)
            wrench::S4U_Mailbox::getMessage(this->test->wms2->mailbox_name);
            wrench::S4U_Mailbox::getMessage(this->test->wms2->mailbox_name);

            // One recv (which fails)
            try {
                wrench::S4U_Mailbox::getMessage(this->test->wms2->mailbox_name);
                throw std::runtime_error("Should have gotten a NetworkError");
            } catch (std::shared_ptr<wrench::NetworkError> &e) {
            }

            // Two synchronous recv, network failure
            try {
                wrench::S4U_Mailbox::getMessage(this->test->wms2->mailbox_name);
                throw std::runtime_error("Should have gotten a NetworkError");
            } catch (std::shared_ptr<wrench::NetworkError> &e) {
            }
            try {
                wrench::S4U_Mailbox::getMessage(this->test->wms2->mailbox_name);
                throw std::runtime_error("Should have gotten a NetworkError");
            } catch (std::shared_ptr<wrench::NetworkError> &e) {
            }

            // One asynchronous recv, network failure
            pending_recv = wrench::S4U_Mailbox::igetMessage(this->test->wms2->mailbox_name);
            wrench::Simulation::sleep(10);
            try {
                pending_recv->wait();
                throw std::runtime_error("Should have gotten a NetworkError");
            } catch (std::shared_ptr<wrench::NetworkError> &e) {
            }

        }

        return 0;
    }
};

TEST_F(S4U_MailboxTest, AsynchronousCommunication) {
    DO_TEST_WITH_FORK(do_AsynchronousCommunication_test);
}

void S4U_MailboxTest::do_AsynchronousCommunication_test() {


    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();

    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("s4u_mailbox_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);


    // Create the WMSs
    this->wms1 = simulation->add(new AsynchronousCommunicationTestWMS(this,"Host1"));
    this->wms2 = simulation->add(new AsynchronousCommunicationTestWMS(this,"Host2"));

    // Create a bogus workflow
    auto workflow =  std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
    this->wms1->addWorkflow(workflow.get());
    this->wms2->addWorkflow(workflow.get());

    simulation->launch();

    delete simulation;

    free(argv[0]);
    free(argv);
}