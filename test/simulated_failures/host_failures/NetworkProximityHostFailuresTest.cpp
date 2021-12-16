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

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"
#include "../failure_test_util/ResourceSwitcher.h"

WRENCH_LOG_CATEGORY(network_proximity_failures, "Log category for NetworkProximityHostFailuresTest");


class NetworkProximityHostFailuresTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::Workflow> workflow;

    std::shared_ptr<wrench::DataFile> input_file;
    std::shared_ptr<wrench::DataFile> output_file;
    std::shared_ptr<wrench::WorkflowTask> task;
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;

    std::shared_ptr<wrench::NetworkProximityService> network_proximity_service = nullptr;

    void do_HostFailures_Test();

protected:
    NetworkProximityHostFailuresTest() {

        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();

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

};


/**********************************************************************/
/**  FAILURE TEST                                                    **/
/**********************************************************************/

class NetworkProxFailuresTestWMS : public wrench::ExecutionController {

public:
    NetworkProxFailuresTestWMS(NetworkProximityHostFailuresTest *test,
                               std::string hostname) :
            wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:

    NetworkProximityHostFailuresTest *test;

    int main() {

        // Starting a FailedHost1 murderer!!
        auto murderer = std::shared_ptr<wrench::ResourceSwitcher>(new wrench::ResourceSwitcher("StableHost", 100, "FailedHost1",
                                                                                               wrench::ResourceSwitcher::Action::TURN_OFF, wrench::ResourceSwitcher::ResourceType::HOST));
        murderer->setSimulation(this->simulation);
        murderer->start(murderer, true, false); // Daemonized, no auto-restart

        // Starting a FailedHost1 resurector!!
        auto resurector = std::shared_ptr<wrench::ResourceSwitcher>(new wrench::ResourceSwitcher("StableHost", 300, "FailedHost1",
                                                                                                 wrench::ResourceSwitcher::Action::TURN_ON, wrench::ResourceSwitcher::ResourceType::HOST));
        resurector->setSimulation(this->simulation);
        resurector->start(resurector, true, false); // Daemonized, no auto-restart

        std::pair<std::string, std::string> first_pair_to_compute_proximity;
        first_pair_to_compute_proximity = std::make_pair("FailedHost1", "FailedHost2");

        wrench::Simulation::sleep(80);
        auto result1 = this->test->network_proximity_service->getHostPairDistance(
                first_pair_to_compute_proximity);
        // Check that timestamp isn't more than 20 seconds past
        std::cerr << "HERE2\n";
        WRENCH_INFO("################ prox=%lf timestamp=%lf",result1.first, result1.second);
        std::cerr << "HERE3\n";
        if (wrench::Simulation::getCurrentSimulatedDate() - result1.second > 20) {
            throw std::runtime_error("Network proximity timestamp shouldn't be more than 20 seconds old");
        }

        wrench::Simulation::sleep(221);
        auto result2 = this->test->network_proximity_service->getHostPairDistance(
                first_pair_to_compute_proximity);
//        WRENCH_INFO("################ prox=%lf timestamp=%lf",result2.first, result2.second);
        // Check that timestamp is more than 120 seconds past
        if (wrench::Simulation::getCurrentSimulatedDate() - result2.second < 120) {
            throw std::runtime_error("Network proximity timestamp shouldn't be less than 120 seconds old");
        }

        wrench::Simulation::sleep(1003);
        auto result3 = this->test->network_proximity_service->getHostPairDistance(
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

TEST_F(NetworkProximityHostFailuresTest, FailingRestartingDaemons) {
    DO_TEST_WITH_FORK(do_HostFailures_Test);
}

void NetworkProximityHostFailuresTest::do_HostFailures_Test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 3;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");
    argv[2] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    std::cerr << "HERE\n";

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string stable_hostname = "StableHost";

//    std::vector<std::string> hosts_in_network = {"FailedHost1", "FailedHost2", "FailedHost3"};
    std::vector<std::string> hosts_in_network = {"FailedHost1", "FailedHost2"};

    ASSERT_NO_THROW(network_proximity_service = simulation->add(
            new wrench::NetworkProximityService(
                    stable_hostname, hosts_in_network, {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_MEASUREMENT_PERIOD,"10"}})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new NetworkProxFailuresTestWMS(this, stable_hostname)));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());

    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}

