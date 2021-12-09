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

WRENCH_LOG_CATEGORY(network_proximity_service_test, "Log category for NetworkProximityTest");


class NetworkProximityTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::DataFile> input_file;
    std::shared_ptr<wrench::DataFile> output_file;
    std::shared_ptr<wrench::WorkflowTask> task;
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;

    void do_NetworkProximity_Test();

    void do_CompareNetworkProximity_Test();

    void do_VivaldiConverge_Test();

    void do_ValidateProperties_Test();

protected:
    NetworkProximityTest() {

        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();

        // Create two files
        input_file = workflow->addFile("input_file", 10000.0);
        output_file = workflow->addFile("output_file", 20000.0);

        // Create one task1
        task = workflow->addTask("task1", 3600, 1, 1, 0 );
        task->addInputFile(input_file);
        task->addOutputFile(output_file);

        // Create a one-host platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\" > "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"101B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>  "
                          "       <host id=\"Host2\" speed=\"1f\" core=\"10\" > "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"101B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>  "
                          "       <host id=\"Host3\" speed=\"1f\" core=\"10\" > "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"101B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>  "
                          "       <host id=\"Host4\" speed=\"1f\" core=\"10\" > "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"101B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>  "
                          "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <link id=\"2\" bandwidth=\"1000GBps\" latency=\"1000us\"/>"
                          "       <link id=\"3\" bandwidth=\"2000GBps\" latency=\"0us\"/>"
                          "       <link id=\"4\" bandwidth=\"3000GBps\" latency=\"0us\"/>"
                          "       <link id=\"5\" bandwidth=\"8000GBps\" latency=\"0us\"/>"
                          "       <link id=\"6\" bandwidth=\"2900GBps\" latency=\"0us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\""
                          "/> </route>"
                          "       <route src=\"Host1\" dst=\"Host3\"> <link_ctn id=\"3\""
                          "/> </route>"
                          "       <route src=\"Host1\" dst=\"Host4\"> <link_ctn id=\"4\""
                          "/> </route>"
                          "       <route src=\"Host2\" dst=\"Host4\"> <link_ctn id=\"5\""
                          "/> </route>"
                          "       <route src=\"Host2\" dst=\"Host3\"> <link_ctn id=\"6\""
                          "/> </route>"
                          "       <route src=\"Host3\" dst=\"Host4\"> <link_ctn id=\"2\""
                          "/> </route>"
                          "   </zone> "
                          "</platform>";

        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    std::shared_ptr<wrench::Workflow> workflow;

};

/**********************************************************************/
/**  SIMPLE PROXIMITY TEST                                           **/
/**********************************************************************/

class ProxTestWMS : public wrench::WMS {

public:
    ProxTestWMS(NetworkProximityTest *test,
                const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                const std::set<std::shared_ptr<wrench::NetworkProximityService>> &network_proximity_services,
                std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, network_proximity_services, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    NetworkProximityTest *test;

    int main() {

        std::pair<std::string, std::string> hosts_to_compute_proximity;
        hosts_to_compute_proximity = std::make_pair(wrench::Simulation::getHostnameList()[2],
                                                    wrench::Simulation::getHostnameList()[1]);
        int count = 0, max_count = 100;
        auto network_proximity_services = this->getAvailableNetworkProximityServices();
        auto network_proximity_service = network_proximity_services.begin();
        double proximity = (*network_proximity_service)->getHostPairDistance(hosts_to_compute_proximity).first;

        while (proximity == DBL_MAX && count < max_count) {
            count++;
            wrench::S4U_Simulation::sleep(20.0);
            proximity = (*network_proximity_service)->getHostPairDistance(hosts_to_compute_proximity).first;
        }

        if (count == max_count) {
            throw std::runtime_error("Never got an answer to proximity query");
        }

        if (proximity == DBL_MAX) {
            throw std::runtime_error("Got a NOT_AVAILABLE proximity value");
        }

        // Shutdown the proximity service
        (*network_proximity_service)->stop();

        try {
            (*network_proximity_service)->getHostPairDistance(hosts_to_compute_proximity);
            throw std::runtime_error("Should not be able to query a service that is down");
        } catch (wrench::ExecutionException &e) {
            // Check Exception
            auto cause = std::dynamic_pointer_cast<wrench::ServiceIsDown>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Got an exception, as expected, but of the unexpected failure cause: " +
                                         e.getCause()->toString() + " (was expecting ServiceIsDown)");
            }
            // Check Exception details
            if (cause->getService() != (*network_proximity_service)) {
                throw std::runtime_error(
                        "Got the expected 'service is down' exception, but the failure cause does not point to the correct service");
            }
        }

        return 0;
    }
};

TEST_F(NetworkProximityTest, NetworkProximity) {
    DO_TEST_WITH_FORK(do_NetworkProximity_Test);
}

void NetworkProximityTest::do_NetworkProximity_Test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname,
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                {})));
    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"})));


    // Create a file registry service
    auto file_registry_service = simulation->add( new wrench::FileRegistryService(hostname));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Get a host for network proximity host
    std::string network_proximity_db_hostname = wrench::Simulation::getHostnameList()[1];

    //Get two hosts to communicate with each other for proximity value
    std::string network_daemon1 = wrench::Simulation::getHostnameList()[0];
    std::string network_daemon2 = wrench::Simulation::getHostnameList()[1];
    std::string network_daemon3 = wrench::Simulation::getHostnameList()[2];
    std::string network_daemon4 = wrench::Simulation::getHostnameList()[3];
    std::vector<std::string> hosts_in_network = {network_daemon1, network_daemon2, network_daemon3, network_daemon4};

    std::shared_ptr<wrench::NetworkProximityService> network_proximity_service = nullptr;

    // A few bogus constructor calls
    ASSERT_THROW(
            new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                                {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_MEASUREMENT_PERIOD, "BOGUS"}}),
            std::invalid_argument);

    ASSERT_THROW(
            new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                                {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "BOGUS"}}),
            std::invalid_argument);

    ASSERT_THROW(
            new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                                {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_MESSAGE_SIZE, "-1.0"}}),
            std::invalid_argument);


    ASSERT_THROW(
            new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                                {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "ALLTOALL"},
                                                 {wrench::NetworkProximityServiceProperty::NETWORK_DAEMON_COMMUNICATION_COVERAGE, "0.5"}}),
            std::invalid_argument);

    std::vector<std::string> too_few_hosts = {network_daemon1};
    ASSERT_THROW(
            new wrench::NetworkProximityService(network_proximity_db_hostname, too_few_hosts,
                                                {}),
            std::invalid_argument);

    // Create a network proximity service with BOGUS Payloads
    ASSERT_THROW(new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network, {}, {{wrench::NetworkProximityServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, -1.0}}), std::invalid_argument);

    ASSERT_THROW(new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network, {}, {{wrench::NetworkProximityServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD, -1.0}}), std::invalid_argument);

    ASSERT_THROW(new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network, {}, {{wrench::NetworkProximityServiceMessagePayload::NETWORK_DAEMON_CONTACT_ANSWER_PAYLOAD, -1.0}}), std::invalid_argument);

    ASSERT_THROW(new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network, {}, {{wrench::NetworkProximityServiceMessagePayload::NETWORK_DAEMON_CONTACT_REQUEST_PAYLOAD, -1.0}}), std::invalid_argument);

    ASSERT_THROW(new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network, {}, {{wrench::NetworkProximityServiceMessagePayload::NETWORK_DAEMON_MEASUREMENT_REPORTING_PAYLOAD, -1.0}}), std::invalid_argument);

    ASSERT_THROW(new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network, {}, {{wrench::NetworkProximityServiceMessagePayload::NETWORK_DB_LOOKUP_REQUEST_MESSAGE_PAYLOAD, -1.0}}), std::invalid_argument);

    ASSERT_THROW(new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network, {}, {{wrench::NetworkProximityServiceMessagePayload::NETWORK_DB_LOOKUP_ANSWER_MESSAGE_PAYLOAD, -1.0}}), std::invalid_argument);


    ASSERT_NO_THROW(network_proximity_service = simulation->add(
            new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                                {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "ALLTOALL"}}, {})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new ProxTestWMS(
                    this,
                    {compute_service}, {storage_service1},
                    {network_proximity_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));


    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());



    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  COMPARE PROXIMITY TEST                                          **/
/**********************************************************************/

class CompareProxTestWMS : public wrench::WMS {

public:
    CompareProxTestWMS(NetworkProximityTest *test,
                       std::set<std::shared_ptr<wrench::ComputeService>> compute_services,
                       std::set<std::shared_ptr<wrench::StorageService>> storage_services,
                       std::set<std::shared_ptr<wrench::NetworkProximityService>> network_proximity_services,
                       std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services,
                        network_proximity_services, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    NetworkProximityTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();


        std::pair<std::string, std::string> first_pair_to_compute_proximity;
        first_pair_to_compute_proximity = std::make_pair(wrench::Simulation::getHostnameList()[0],
                                                         wrench::Simulation::getHostnameList()[1]);
        int count = 0, max_count = 1000;
        double first_pair_proximity = (*(this->getAvailableNetworkProximityServices().begin()))->getHostPairDistance(
                first_pair_to_compute_proximity).first;

        while (first_pair_proximity == DBL_MAX && count < max_count) {
            count++;
            wrench::Simulation::sleep(10.0);
            first_pair_proximity = (*(this->getAvailableNetworkProximityServices().begin()))->getHostPairDistance(
                    first_pair_to_compute_proximity).first;
        }

        if (count == max_count) {
            throw std::runtime_error("Never got an answer to proximity query");
        }

        if (first_pair_proximity == DBL_MAX) {
            throw std::runtime_error("Got a NOT_AVAILABLE proximity value");
        }


        std::pair<std::string, std::string> second_pair_to_compute_proximity;
        second_pair_to_compute_proximity = std::make_pair(wrench::Simulation::getHostnameList()[2],
                                                          wrench::Simulation::getHostnameList()[3]);
        count = 0, max_count = 1000;
        double second_pair_proximity = (*(this->getAvailableNetworkProximityServices().begin()))->getHostPairDistance(
                second_pair_to_compute_proximity).first;

        while (second_pair_proximity == DBL_MAX && count < max_count) {
            count++;
            wrench::Simulation::sleep(10.0);
            second_pair_proximity = (*(this->getAvailableNetworkProximityServices().begin()))->getHostPairDistance(
                    second_pair_to_compute_proximity).first;
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
        return 0;
    }
};

TEST_F(NetworkProximityTest, CompareNetworkProximity) {
    DO_TEST_WITH_FORK(do_CompareNetworkProximity_Test);
}

void NetworkProximityTest::do_CompareNetworkProximity_Test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];



    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname,
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                {})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"})));



    simulation->add(new wrench::FileRegistryService(hostname));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Get a host for network proximity host
    std::string network_proximity_db_hostname = wrench::Simulation::getHostnameList()[1];

    //Get two hosts to communicate with each other for proximity value
    std::string network_daemon1 = wrench::Simulation::getHostnameList()[0];
    std::string network_daemon2 = wrench::Simulation::getHostnameList()[1];
    std::string network_daemon3 = wrench::Simulation::getHostnameList()[2];
    std::string network_daemon4 = wrench::Simulation::getHostnameList()[3];
    std::vector<std::string> hosts_in_network = {network_daemon1, network_daemon2, network_daemon3, network_daemon4};

    std::shared_ptr<wrench::NetworkProximityService> network_proximity_service;

    ASSERT_NO_THROW(network_proximity_service = simulation->add(new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network, {})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new CompareProxTestWMS(this, (std::set<std::shared_ptr<wrench::ComputeService>>){compute_service},
                                   (std::set<std::shared_ptr<wrench::StorageService>>){storage_service1},
                                   (std::set<std::shared_ptr<wrench::NetworkProximityService>>){network_proximity_service},
                                   hostname)));

    wms->addWorkflow(this->workflow, 0.0);



    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());



    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  VIVALDI CONVERGE TEST                                           **/
/**********************************************************************/

class VivaldiConvergeWMS : public wrench::WMS {
public:
    VivaldiConvergeWMS(NetworkProximityTest *test,
                       std::set<std::shared_ptr<wrench::ComputeService>> compute_services,
                       std::set<std::shared_ptr<wrench::StorageService>> storage_services,
                       std::set<std::shared_ptr<wrench::NetworkProximityService>> network_proximity_services,
                       std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services,
                        network_proximity_services, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    NetworkProximityTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        std::pair<std::string, std::string> hosts_to_compute_proximity;
        hosts_to_compute_proximity = std::make_pair("Host3", "Host4");

        auto network_proximity_services = this->getAvailableNetworkProximityServices();

        std::shared_ptr<wrench::NetworkProximityService> alltoall_service;
        std::shared_ptr<wrench::NetworkProximityService> vivaldi_service;

        for (auto  &nps : network_proximity_services) {
            std::string type = nps->getNetworkProximityServiceType();

            if (boost::iequals(type, "alltoall")) {
                alltoall_service = nps;
            }

            if (boost::iequals(type, "vivaldi")) {
                vivaldi_service = nps;
            }
        }

        wrench::S4U_Simulation::sleep(1000);
        double alltoall_proximity = alltoall_service->getHostPairDistance(hosts_to_compute_proximity).first;
        double vivaldi_proximity = vivaldi_service->getHostPairDistance(hosts_to_compute_proximity).first;


        if (vivaldi_proximity > 0.1) {
            throw std::runtime_error("Vivaldi proximity is larger than it should be");
        }

        // Check values
        double epsilon = 0.1;

        if (std::abs(vivaldi_proximity - alltoall_proximity) > epsilon) {
            throw std::runtime_error("Vivaldi algorithm did not converge");
        }

        std::string target_host = "Host3";
        std::pair<double,double> coordinates = vivaldi_service->getHostCoordinate(target_host).first;

        if (coordinates.first == 0 && coordinates.second == 0) {
            throw std::runtime_error("Vivaldi algorithm did not update the coordinates of host:" + target_host);
        }

        // Try to get coordinates from a service that does not support coordinates
        try {
            coordinates = alltoall_service->getHostCoordinate(target_host).first;
            throw std::runtime_error(
                    "Should not be able to get coordinates from an all-to-all proximity service");
        } catch (std::runtime_error &e) {
        }

        // stop the service
        vivaldi_service->stop();
        try {
            coordinates = vivaldi_service->getHostCoordinate(target_host).first;
            throw std::runtime_error("Should not be able to get coordinates from a service that is down");
        } catch (wrench::ExecutionException &e) {
            // Check Exception
            auto cause = std::dynamic_pointer_cast<wrench::ServiceIsDown>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Got an exception, as expected, but of the unexpected failure cause: " +
                                         e.getCause()->toString() + " (was expecting ServiceIsDown)");
            }
            // Check Exception details
            wrench::ServiceIsDown *real_cause = (wrench::ServiceIsDown *) e.getCause().get();
            if (real_cause->getService() != vivaldi_service) {
                throw std::runtime_error(
                        "Got the expected 'service is down' exception, but the failure cause does not point to the correct service");
            }
        }

        return 0;
    }
};

TEST_F(NetworkProximityTest, VivaldiConvergeTest) {
    DO_TEST_WITH_FORK(do_VivaldiConverge_Test);
}

void NetworkProximityTest::do_VivaldiConverge_Test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname,
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                {})));
    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"})));



    // Create a file registry service
    simulation->add(new wrench::FileRegistryService(hostname));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Get a host for network proximity host
    std::string network_proximity_db_hostname = wrench::Simulation::getHostnameList()[1];

    //Get two hosts to communicate with each other for proximity value
    std::string host1 = wrench::Simulation::getHostnameList()[0];
    std::string host2 = wrench::Simulation::getHostnameList()[1];
    std::string host3 = wrench::Simulation::getHostnameList()[2];
    std::string host4 = wrench::Simulation::getHostnameList()[3];
    std::vector<std::string> hosts_in_network = {host1, host2, host3, host4};

    std::shared_ptr<wrench::NetworkProximityService> alltoall_network_service = nullptr;
    std::shared_ptr<wrench::NetworkProximityService> vivaldi_network_service = nullptr;

    // Add vivaldi and alltoall network proximity services
    ASSERT_NO_THROW(alltoall_network_service = simulation->add(
            new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                                {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "ALLTOALL"}})));

    ASSERT_NO_THROW(vivaldi_network_service = simulation->add(
            new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                                {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "VIVALDI"},
                                                 {wrench::NetworkProximityServiceProperty::NETWORK_DAEMON_COMMUNICATION_COVERAGE, "1.0"}})));
    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new VivaldiConvergeWMS(
                    this,
                    (std::set<std::shared_ptr<wrench::ComputeService>>){compute_service},
                    (std::set<std::shared_ptr<wrench::StorageService>>){storage_service1},
                    (std::set<std::shared_ptr<wrench::NetworkProximityService>>){alltoall_network_service, vivaldi_network_service},
                    hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());



    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  VALIDATE PROPERTIES TEST                                        **/
/**********************************************************************/

class ValidatePropertiesWMS : public wrench::WMS {
public:
    ValidatePropertiesWMS(NetworkProximityTest *test,
                          std::set<std::shared_ptr<wrench::ComputeService>> compute_services,
                          std::set<std::shared_ptr<wrench::StorageService>> storage_services,
                          std::set<std::shared_ptr<wrench::NetworkProximityService>> network_proximity_services,
                          std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services,
                        network_proximity_services, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    NetworkProximityTest *test;

    int main() {
//      wrench::S4U_Simulation::sleep(10);
        return 0;
    }
};

TEST_F(NetworkProximityTest, NetworkProximityValidatePropertiesTest) {
    DO_TEST_WITH_FORK(do_ValidateProperties_Test);
}

void NetworkProximityTest::do_ValidateProperties_Test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname,
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                {})));
    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"})));

    // Create a file registry service
    simulation->add(new wrench::FileRegistryService(hostname));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Get a host for network proximity host
    std::string network_proximity_db_hostname = wrench::Simulation::getHostnameList()[1];

    //Get two hosts to communicate with each other for proximity value
    std::string host1 = wrench::Simulation::getHostnameList()[0];
    std::string host2 = wrench::Simulation::getHostnameList()[1];
    std::vector<std::string> hosts_in_network = {host1, host2};

    std::shared_ptr<wrench::NetworkProximityService> nps = nullptr;
    ASSERT_NO_THROW(nps = simulation->add(new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                                                              {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "ALLTOALL"}})));

    ASSERT_THROW(nps = simulation->add(
            new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                                {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "ALLTOALL"},
                                                 {wrench::NetworkProximityServiceProperty::NETWORK_DAEMON_COMMUNICATION_COVERAGE, "0.5"}})),
                 std::invalid_argument);

    ASSERT_THROW(nps = simulation->add(
            new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                                {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "VIVALDI"},
                                                 {wrench::NetworkProximityServiceProperty::NETWORK_DAEMON_COMMUNICATION_COVERAGE, "1.1"}})),
                 std::invalid_argument);

    ASSERT_THROW(nps = simulation->add(
            new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                                {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "VIVALDI"},
                                                 {wrench::NetworkProximityServiceProperty::NETWORK_DAEMON_COMMUNICATION_COVERAGE, "-1.1"}})),
                 std::invalid_argument);

    ASSERT_THROW(nps = simulation->add(
            new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                                {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "ALLTOALL"}},
                                                {{wrench::NetworkProximityServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD, -1.0}})),
                 std::invalid_argument);

    ASSERT_THROW(nps = simulation->add(
            new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                                {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "ALLTOALL"}},
                                                {{wrench::NetworkProximityServiceMessagePayload::NETWORK_DB_LOOKUP_REQUEST_MESSAGE_PAYLOAD, -1.0}})),
                 std::invalid_argument);

    ASSERT_THROW(nps = simulation->add(
            new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                                {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "ALLTOALL"}},
                                                {{wrench::NetworkProximityServiceMessagePayload::NETWORK_DB_LOOKUP_ANSWER_MESSAGE_PAYLOAD, -1.0}})),
                 std::invalid_argument);

    ASSERT_THROW(nps = simulation->add(
            new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                                {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "ALLTOALL"}},
                                                {{wrench::NetworkProximityServiceMessagePayload::NETWORK_DAEMON_CONTACT_ANSWER_PAYLOAD, -1.0}})),
                 std::invalid_argument);

    ASSERT_THROW(nps = simulation->add(
            new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                                {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "ALLTOALL"},
                                                 {wrench::NetworkProximityServiceProperty::LOOKUP_OVERHEAD, "-1.0"}})),
                 std::invalid_argument);

    ASSERT_THROW(nps = simulation->add(
            new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                                {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "ALLTOALL"},
                                                 {wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_MESSAGE_SIZE, "-1.0"}})),
                 std::invalid_argument);

    ASSERT_THROW(nps = simulation->add(
            new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                                {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "ALLTOALL"},
                                                 {wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_MEASUREMENT_PERIOD, "-1.0"}})),
                 std::invalid_argument);


    ASSERT_THROW(nps = simulation->add(
            new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                                {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "ALLTOALL"},
                                                 {wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_MEASUREMENT_PERIOD_MAX_NOISE, "-1.0"}})),
                 std::invalid_argument);


    ASSERT_NO_THROW(nps = simulation->add(
            new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                                {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "VIVALDI"},
                                                 {wrench::NetworkProximityServiceProperty::NETWORK_DAEMON_COMMUNICATION_COVERAGE, "0.01"},
                                                 {wrench::NetworkProximityServiceProperty::LOOKUP_OVERHEAD, "4.0"},
                                                 {wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_MESSAGE_SIZE, "20.1"},
                                                 {wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_MEASUREMENT_PERIOD, "0.01"},
                                                 {wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_MEASUREMENT_PERIOD_MAX_NOISE, "10"}},
                                                {{wrench::NetworkProximityServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD, 1},
                                                 {wrench::NetworkProximityServiceMessagePayload::NETWORK_DB_LOOKUP_REQUEST_MESSAGE_PAYLOAD, 2},
                                                 {wrench::NetworkProximityServiceMessagePayload::NETWORK_DB_LOOKUP_ANSWER_MESSAGE_PAYLOAD, 2},
                                                 {wrench::NetworkProximityServiceMessagePayload::NETWORK_DAEMON_CONTACT_ANSWER_PAYLOAD, 3.1}}

            )));

    //Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new ValidatePropertiesWMS(
                    this,
                    (std::set<std::shared_ptr<wrench::ComputeService>>){compute_service},
                    (std::set<std::shared_ptr<wrench::StorageService>>){storage_service1},
                    (std::set<std::shared_ptr<wrench::NetworkProximityService>>){nps},
                    hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

//  // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());




    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}