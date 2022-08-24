/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"

class SimulationPlatformTest : public ::testing::Test {

public:
    void do_SimulationPlatformTest_test();
    void do_CreateNewDiskTest_test();
    void do_ProgrammaticPlatformTest_test();

protected:
    ~SimulationPlatformTest() {
    }

    SimulationPlatformTest() {
        // Create a platform file
        std::string xml = R"(<?xml version='1.0' encoding='utf-8'?>
<!DOCTYPE platform SYSTEM "https://simgrid.org/simgrid.dtd">
<platform version="4.1">
<zone id="world" routing="Full">

    <cluster bw="125MBps" id="simple" lat="50us" prefix="node-" radical="0-7" speed="1Gf" suffix=".1core.org">
        <prop id="wattage_per_state" value="0.0:1.0:1.0" />
        <prop id="wattage_off" value="0.0" />
    </cluster>

    <cluster bb_bw="2.25GBps" bb_lat="500us" bw="125MBps" core="2" id="backboned" lat="50us" prefix="node-" radical="0-7" speed="1Gf" suffix=".2cores.org">
        <prop id="wattage_per_state" value="0.0:0.0:2.0" />
        <prop id="wattage_off" value="0.0" />
    </cluster>

    <zone id="host_zone" routing="Full">
        <host id="subzonehost" speed="1f" core="2"/>
    </zone>

    <zone id="subzone" routing="Full">
        <cluster bb_bw="2.25GBps" bb_lat="500us" bb_sharing_policy="SHARED" bw="125MBps" core="4" id="halfduplex" lat="50us" prefix="node-" radical="0-3" sharing_policy="SHARED" speed="1Gf" suffix=".4cores.org">
            <prop id="wattage_per_state" value="0.0:0.0:4.0" />
            <prop id="wattage_off" value="0.0" />
        </cluster>
    </zone>
</zone>
</platform>
)";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};

/**********************************************************************/
/**            SIMULATION PLATFORM TEST                              **/
/**********************************************************************/

class SimulationPlatformTestWMS : public wrench::ExecutionController {

public:
    SimulationPlatformTestWMS(SimulationPlatformTest *test,
                              std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    SimulationPlatformTest *test;

    int main() {

        // Testing finding subzones
        auto subnetzones = wrench::S4U_Simulation::getAllSubZoneIDsByZone();
        if (subnetzones.size() != 1) {
            throw std::runtime_error("Invalid number of netzones found");
        }
        if (subnetzones["world"].size() != 2) {
            throw std::runtime_error("Invalid number of netzones found under the 'world' netzone");
        }
        if (std::find(subnetzones["world"].begin(), subnetzones["world"].end(), "subzone") == subnetzones["world"].end()) {
            throw std::runtime_error("Should find netzone 'subzone' in netzone 'world'");
        }
        if (std::find(subnetzones["world"].begin(), subnetzones["world"].end(), "host_zone") == subnetzones["world"].end()) {
            throw std::runtime_error("Should find netzone 'host_zone' in netzone 'world'");
        }


        // Testing finding clusters
        auto clusters = wrench::S4U_Simulation::getAllClusterIDsByZone();
        if (clusters.size() != 2) {
            throw std::runtime_error("Invalid number of netzones found");
        }
        if (clusters["world"].size() != 2) {
            throw std::runtime_error("Invalid number of clusters found under the 'world' netzone");
        }
        if (std::find(clusters["world"].begin(), clusters["world"].end(), "simple") == clusters["world"].end()) {
            throw std::runtime_error("Should find cluster 'simple' in netzone 'world'");
        }
        if (std::find(clusters["world"].begin(), clusters["world"].end(), "backboned") == clusters["world"].end()) {
            throw std::runtime_error("Should find cluster 'backboned' in netzone 'world'");
        }
        if (clusters["subzone"].size() != 1) {
            throw std::runtime_error("Invalid number of cluster found under the 'subzone' netzone");
        }
        if (std::find(clusters["subzone"].begin(), clusters["subzone"].end(), "halfduplex") == clusters["subzone"].end()) {
            throw std::runtime_error("Should find cluster 'halfduplex' in netzone 'subzone'");
        }

        // Testing finding hosts in zones
        auto hosts = wrench::S4U_Simulation::getAllHostnamesByZone();
        if (hosts.size() != 1) {
            throw std::runtime_error("Invalid number of netzones found");
        }
        if (hosts["host_zone"].size() != 1) {
            throw std::runtime_error("Invalid number of hosts found under zone 'host_zone' found");
        }

        // Testing finding hosts in clusters
        hosts = wrench::S4U_Simulation::getAllHostnamesByCluster();
        if (hosts.size() != 3) {
            throw std::runtime_error("Invalid number of clusters found");
        }
        if (hosts["simple"].size() != 8) {
            throw std::runtime_error("Invalid number of hosts found under cluster 'simple' found");
        }
        if (hosts["backboned"].size() != 8) {
            throw std::runtime_error("Invalid number of hosts found under cluster 'backboned' found");
        }
        if (hosts["halfduplex"].size() != 4) {
            throw std::runtime_error("Invalid number of hosts found under cluster 'halfduplex' found");
        }

        if (wrench::S4U_Simulation::getClusterProperty("simple", "wattage_off") != "0.0") {
            throw std::runtime_error("Invalid cluster property value");
        }

        return 0;
    }
};

TEST_F(SimulationPlatformTest, SimulationPlatformTestWMS) {
    DO_TEST_WITH_FORK(do_SimulationPlatformTest_test);
}

void SimulationPlatformTest::do_SimulationPlatformTest_test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    //    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string hostname = "subzonehost";

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(new SimulationPlatformTestWMS(this, hostname)));

    // Running the simulation
    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**            CREATE DISK TEST                                      **/
/**********************************************************************/

class CreateNewDiskTestWMS : public wrench::ExecutionController {

public:
    CreateNewDiskTestWMS(SimulationPlatformTest *test,
                         std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    SimulationPlatformTest *test;

    int main() override {

        try {
            wrench::S4U_Simulation::createNewDisk("subzonehost", "new_disk", 10.0, 20.0, 100.0, "/foo");
            throw std::runtime_error("Should not be able to create a disk with different read and write bandwidths");
        } catch (std::invalid_argument &ignore) {}

        // Create a new disk on subzonehost
        wrench::S4U_Simulation::createNewDisk("subzonehost", "new_disk", 10.0, 10.0, 100.0, "/foo");

        // Start a storage service that uses this disk
        auto ss = this->simulation->startNewService(new wrench::SimpleStorageService("subzonehost", {"/foo"}, {}, {}));

        // Create a file on it
        auto too_big = wrench::Simulation::addFile("too_big", 200.0);
        try {
            wrench::Simulation::createFile(too_big, wrench::FileLocation::LOCATION(ss));
            throw std::runtime_error("Should not be able to create a file that big on the newly created storage service");
        } catch (std::invalid_argument &ignore) {}
        auto not_too_big = wrench::Simulation::addFile("not_too_big", 20.0);
        wrench::Simulation::createFile(not_too_big, wrench::FileLocation::LOCATION(ss));

        return 0;
    }
};

TEST_F(SimulationPlatformTest, CreateNewDiskTestWMS) {
    DO_TEST_WITH_FORK(do_CreateNewDiskTest_test);
}

void SimulationPlatformTest::do_CreateNewDiskTest_test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    //    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string hostname = "subzonehost";

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(new CreateNewDiskTestWMS(this, hostname)));

    // Running the simulation
    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**            PROGRAMMATIC PLATFORM TEST                            **/
/**********************************************************************/

class PlatformCreator {

public:
    PlatformCreator(double link_bw) : link_bw(link_bw) {}

    void operator()() const {
        create_platform(this->link_bw);
    }

private:
    double link_bw;

    void create_platform(double link_bw) const {
        // Create the top-level zone
        auto zone = simgrid::s4u::create_full_zone("AS0");
        // Create the WMSHost host with its disk
        auto wms_host = zone->create_host("WMSHost", "10Gf");
        wms_host->set_core_count(1);
        auto wms_host_disk = wms_host->create_disk("hard_drive",
                                                   "100MBps",
                                                   "100MBps");
        wms_host_disk->set_property("size", "5000GiB");
        wms_host_disk->set_property("mount", "/");

        // Create a ComputeHost
        auto compute_host = zone->create_host("ComputeHost", "1Gf");
        compute_host->set_core_count(10);
        compute_host->set_property("ram", "16GB");

        // Create three network links
        auto network_link = zone->create_link("network_link", link_bw)->set_latency("20us");
        auto loopback_WMSHost = zone->create_link("loopback_WMSHost", "1000EBps")->set_latency("0us");
        auto loopback_ComputeHost = zone->create_link("loopback_ComputeHost", "1000EBps")->set_latency("0us");

        // Add routes
        {
            simgrid::s4u::LinkInRoute network_link_in_route{network_link};
            zone->add_route(compute_host->get_netpoint(),
                            wms_host->get_netpoint(),
                            nullptr,
                            nullptr,
                            {network_link_in_route});
        }
        {
            simgrid::s4u::LinkInRoute network_link_in_route{loopback_WMSHost};
            zone->add_route(wms_host->get_netpoint(),
                            wms_host->get_netpoint(),
                            nullptr,
                            nullptr,
                            {network_link_in_route});
        }
        {
            simgrid::s4u::LinkInRoute network_link_in_route{loopback_ComputeHost};
            zone->add_route(compute_host->get_netpoint(),
                            compute_host->get_netpoint(),
                            nullptr,
                            nullptr,
                            {network_link_in_route});
        }

        zone->seal();
    }
};


TEST_F(SimulationPlatformTest, ProgrammaticPlatform) {
    DO_TEST_WITH_FORK(do_ProgrammaticPlatformTest_test);
}

void SimulationPlatformTest::do_ProgrammaticPlatformTest_test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    PlatformCreator platform_creator(100 * 1000000.0);
    simulation->instantiatePlatform(platform_creator);

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
