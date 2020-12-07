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
#include <wrench/services/helpers/HostStateChangeDetectorMessage.h>
#include "../../include/UniqueTmpPathPrefix.h"
#include "../../include/TestWithFork.h"

WRENCH_LOG_CATEGORY(host_state_change_detector_service_test, "Log category for HostStateChangeDetectorService test");


class HostStateChangeDetectorServiceTest : public ::testing::Test {

public:


    void do_StateChangeDetection_test(bool notify_when_speed_change);

protected:
    HostStateChangeDetectorServiceTest() {

        // Create a two-host platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"Host2\" speed=\"1f, 2f\" pstate=\"0\" core=\"1\"/> "
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
/**  HOST STATE CHANGE DETECTOR SERVICE TEST                         **/
/**********************************************************************/

class HostStateChangeDetectorTestWMS : public wrench::WMS {

public:
    HostStateChangeDetectorTestWMS(HostStateChangeDetectorServiceTest *test,
                                   std::string hostname, bool notify_when_speed_change) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
        this->notify_when_speed_change = notify_when_speed_change;
    }


private:

    HostStateChangeDetectorServiceTest *test;
    bool notify_when_speed_change;

    int main() {

        // Create a StateChangeDetector
        std::vector<std::string> hosts;
        hosts.push_back("Host2");
        auto ssd = std::shared_ptr<wrench::HostStateChangeDetector>(
                new wrench::HostStateChangeDetector(this->hostname, hosts, true, true, this->notify_when_speed_change,
                                                    this->getSharedPtr<wrench::WMS>(), this->mailbox_name, {}));
        ssd->simulation = this->simulation;
        ssd->start(ssd, true, false);

        wrench::Simulation::sleep(10);
        wrench::Simulation::turnOffHost("Host2");

        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = wrench::S4U_Mailbox::getMessage(this->mailbox_name, 10);
        } catch (std::shared_ptr<wrench::NetworkError> &e) {
            throw std::runtime_error("Did not get a message before the timeout");
        }
        if (not std::dynamic_pointer_cast<wrench::HostHasTurnedOffMessage>(message)) {
            throw std::runtime_error("Did not get the expected 'host has turned off' message");
        }

        wrench::Simulation::sleep(10);
        simgrid::s4u::Host::by_name("Host2")->turn_on();

        try {
            message = wrench::S4U_Mailbox::getMessage(this->mailbox_name, 10);
        } catch (std::shared_ptr<wrench::NetworkError> &e) {
            throw std::runtime_error("Did not get a message before the timeout");
        }
        if (not std::dynamic_pointer_cast<wrench::HostHasTurnedOnMessage>(message)) {
            throw std::runtime_error("Did not get the expected 'host has turned on' message");
        }

        wrench::Simulation::sleep(10);
        this->simulation->setPstate("Host2", 0);
        wrench::Simulation::sleep(10);

        if (this->notify_when_speed_change) {

            try {
                message = wrench::S4U_Mailbox::getMessage(this->mailbox_name, 10);
            } catch (std::shared_ptr<wrench::NetworkError> &e) {
                throw std::runtime_error("Did not get a message before the timeout");
            }
            if (not std::dynamic_pointer_cast<wrench::HostHasChangedSpeedMessage>(message)) {
                throw std::runtime_error("Did not get the expected 'host has changed speed' message");
            }
        }

        ssd->kill(); // coverage


        return 0;
    }
};

TEST_F(HostStateChangeDetectorServiceTest, StateChangeDetectionTest) {
    DO_TEST_WITH_FORK_ONE_ARG(do_StateChangeDetection_test, true);
    DO_TEST_WITH_FORK_ONE_ARG(do_StateChangeDetection_test, false);
}

void HostStateChangeDetectorServiceTest::do_StateChangeDetection_test(bool notify_when_speed_change) {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();

    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("test");
//    argv[1] = strdup("--wrench-log-full");


    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);

    // Create the WMS
    auto  wms = simulation->add(new HostStateChangeDetectorTestWMS(this,"Host1", notify_when_speed_change));

    // Create a bogus workflow
    auto workflow =  std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
    wms->addWorkflow(workflow.get());

    simulation->launch();

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}