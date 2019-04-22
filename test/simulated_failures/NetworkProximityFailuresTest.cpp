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
#include <boost/algorithm/string.hpp>

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"
#include "failure_test_util/HostSwitcher.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(network_proximity_failures, "Log category for NetworkProximityFailuresTest");


class NetworkProximityFailuresTest : public ::testing::Test {

public:
    wrench::WorkflowFile *input_file;
    wrench::WorkflowFile *output_file;
    wrench::WorkflowTask *task;
    wrench::StorageService *storage_service1 = nullptr;
    wrench::ComputeService *compute_service = nullptr;

    void do_NetworkProximityFailures_Test();

protected:
    NetworkProximityFailuresTest() {

        // Create the simplest workflow
        workflow = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());

        // Create a one-host platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"StableHost\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"FailedHost1\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"FailedHost2\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"FailedHost3\" speed=\"1f\" core=\"10\"/> "
                          "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <link id=\"2\" bandwidth=\"1000GBps\" latency=\"1000us\"/>"
                          "       <link id=\"3\" bandwidth=\"2000GBps\" latency=\"0us\"/>"
                          "       <link id=\"4\" bandwidth=\"3000GBps\" latency=\"0us\"/>"
                          "       <link id=\"5\" bandwidth=\"8000GBps\" latency=\"0us\"/>"
                          "       <link id=\"6\" bandwidth=\"2900GBps\" latency=\"0us\"/>"
                          "       <route src=\"StableHost\" dst=\"FailedHost1\"> <link_ctn id=\"1\""
                          "/> </route>"
                          "       <route src=\"StableHost\" dst=\"FailedHost2\"> <link_ctn id=\"3\""
                          "/> </route>"
                          "       <route src=\"StableHost\" dst=\"FailedHost3\"> <link_ctn id=\"4\""
                          "/> </route>"
                          "       <route src=\"FailedHost1\" dst=\"FailedHost3\"> <link_ctn id=\"5\""
                          "/> </route>"
                          "       <route src=\"FailedHost1\" dst=\"FailedHost2\"> <link_ctn id=\"2\""
                          "/> </route>"
                          "       <route src=\"FailedHost2\" dst=\"FailedHost3\"> <link_ctn id=\"2\""
                          "/> </route>"
                          "   </zone> "
                          "</platform>";

        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    std::unique_ptr<wrench::Workflow> workflow;

};


/**********************************************************************/
/**  FAILURE TEST                                                    **/
/**********************************************************************/

class NetworkProxFailuresTestWMS : public wrench::WMS {

public:
    NetworkProxFailuresTestWMS(NetworkProximityFailuresTest *test,
                               std::set<wrench::NetworkProximityService *> network_proximity_services,
                               std::string hostname) :
            wrench::WMS(nullptr, nullptr,  {}, {},
                        network_proximity_services, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    NetworkProximityFailuresTest *test;

    int main() {

        // Starting a FailedHost1 murderer!!
        auto murderer = std::shared_ptr<wrench::HostSwitcher>(new wrench::HostSwitcher("StableHost", 100, "FailedHost1", wrench::HostSwitcher::Action::TURN_OFF));
        murderer->simulation = this->simulation;
        murderer->start(murderer, true, false); // Daemonized, no auto-restart

        // Starting a FailedHost1 resurector!!
        auto resurector = std::shared_ptr<wrench::HostSwitcher>(new wrench::HostSwitcher("StableHost", 300, "FailedHost1", wrench::HostSwitcher::Action::TURN_ON));
        resurector->simulation = this->simulation;
        resurector->start(resurector, true, false); // Daemonized, no auto-restart

        std::pair<std::string, std::string> first_pair_to_compute_proximity;
        first_pair_to_compute_proximity = std::make_pair("FailedHost1", "FailedHost2");

        wrench::Simulation::sleep(80);
        auto result1 = (*(this->getAvailableNetworkProximityServices().begin()))->getHostPairDistance(
                first_pair_to_compute_proximity);
        // Check that timestamp isn't more than 10 seconds past
//        WRENCH_INFO("################ prox=%lf timestamp=%lf",result1.first, result1.second);
        if (wrench::Simulation::getCurrentSimulatedDate() - result1.second > 20) {
            throw std::runtime_error("Network proximity timestamp shouldn't be more than 10 seconds old");
        }

        wrench::Simulation::sleep(221);
        auto result2 = (*(this->getAvailableNetworkProximityServices().begin()))->getHostPairDistance(
                first_pair_to_compute_proximity);
//        WRENCH_INFO("################ prox=%lf timestamp=%lf",result2.first, result2.second);
        // Check that timestamp is more than 120 seconds past
        if (wrench::Simulation::getCurrentSimulatedDate() - result2.second < 120) {
            throw std::runtime_error("Network proximity timestamp shouldn't be less than 120 seconds old");
        }

        wrench::Simulation::sleep(1003);
        auto result3 = (*(this->getAvailableNetworkProximityServices().begin()))->getHostPairDistance(
                first_pair_to_compute_proximity);
//        WRENCH_INFO("################ prox=%lf timestamp=%lf",result3.first, result3.second);
        // Check that timestamp isn't more than 10 seconds past
        if (wrench::Simulation::getCurrentSimulatedDate() - result3.second > 20) {
            throw std::runtime_error("Network proximity timestamp shouldn't be more than 10 seconds old");
        }

#ifdef COMMENTED_OUT_FOR_NOW

        std::pair<std::string, std::string> second_pair_to_compute_proximity;
        second_pair_to_compute_proximity = std::make_pair("FailedHost2", "FailedHost3");
        count = 0, max_count = 1000;
        double second_pair_proximity = (*(this->getAvailableNetworkProximityServices().begin()))->query(
                second_pair_to_compute_proximity);

        while (second_pair_proximity == DBL_MAX && count < max_count) {
            count++;
            wrench::Simulation::sleep(10.0);
            second_pair_proximity = (*(this->getAvailableNetworkProximityServices().begin()))->query(second_pair_to_compute_proximity);
        }

        if (count == max_count) {
            throw std::runtime_error("Never got an answer to proximity query");
        }

        if (second_pair_proximity == DBL_MAX) {
            throw std::runtime_error("Got a NOT_AVAILABLE proximity value");
        }

        if (first_pair_proximity > second_pair_proximity) {
            throw std::runtime_error(
                    "Expected proximity between a pair to be less than the other pair of hosts"
            );
        }
#endif

        return 0;
    }
};

TEST_F(NetworkProximityFailuresTest, FailingRestartingDaemons) {
    DO_TEST_WITH_FORK(do_NetworkProximityFailures_Test);
}

void NetworkProximityFailuresTest::do_NetworkProximityFailures_Test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("one_task_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string stable_hostname = "StableHost";

//    std::vector<std::string> hosts_in_network = {"FailedHost1", "FailedHost2", "FailedHost3"};
    std::vector<std::string> hosts_in_network = {"FailedHost1", "FailedHost2"};

    wrench::NetworkProximityService *network_proximity_service(
            new wrench::NetworkProximityService(stable_hostname, hosts_in_network, {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_MEASUREMENT_PERIOD,"10"},
                                                                                    {wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_MEASUREMENT_PERIOD_MAX_NOISE,"0"}})
    );

    ASSERT_NO_THROW(simulation->add(network_proximity_service));

    // Create a WMS
    wrench::WMS *wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new NetworkProxFailuresTestWMS(this,
                                           (std::set<wrench::NetworkProximityService *>){network_proximity_service},
                                           stable_hostname)));

    wms->addWorkflow(this->workflow.get(), 0.0);



    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}

