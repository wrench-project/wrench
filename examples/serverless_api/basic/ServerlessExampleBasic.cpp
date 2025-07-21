/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** This simulator simulates the execution of function invocations placed
 ** to a serverless compute service.
 **
 ** The compute platform comprises a host that runs the controller (UserHost), a host that serves as the
 ** frontend to the serverless compute service (ServerlessHeadNode), which can use some compute nodes
 ** to run functions (ServerlessComputeNode1, ServerlessComputeNode2, ...). The number of compute nodes
 ** is specified in a command-line arguments, as is the number of function invocations to be placed.
 **
 ** Example invocation of the simulator for 100 function invocations and 3 compute nodes
 **    ./wrench-example-bare-metal-bag-of-actions 100 3
 **/


#include <iostream>
#include <wrench.h>

#include "ServerlessExampleExecutionController.h"
#include "wrench/services/compute/serverless/schedulers/RandomServerlessScheduler.h"
#include "wrench/services/compute/serverless/schedulers/FCFSServerlessScheduler.h"
#include "wrench/services/compute/serverless/schedulers/WorkloadBalancingServerlessScheduler.h"

namespace sg4 = simgrid::s4u;

/**
 * Helper functor class to create the hardware platform (programmatically, instead of via an XML file).
 **/
class PlatformCreator {
public:
    explicit PlatformCreator(unsigned int num_hosts) : num_hosts_(num_hosts) {
    }

    void operator()() const {
        create_platform();
    }

private:
    unsigned int num_hosts_;

    void create_platform() const {
        // Get the top-level zone
        auto zone = simgrid::s4u::Engine::get_instance()->get_netzone_root();

        // Create the WMSHost host with its disk
        auto wms_host = zone->add_host("UserHost", "10Gf");
        wms_host->set_core_count(1);
        auto wms_host_disk = wms_host->add_disk("hard_drive",
                                                "100MBps",
                                                "100MBps");
        wms_host_disk->set_property("size", "5000GB");
        wms_host_disk->set_property("mount", "/");

        // Create the ServerlessHeadNode host with its disk
        auto head_node = zone->add_host("ServerlessHeadNode", "10Gf");
        head_node->set_core_count(1);
        auto head_node_disk = head_node->add_disk("hard_drive",
                                                  "100MBps",
                                                  "100MBps");
        head_node_disk->set_property("size", "5000GB");
        head_node_disk->set_property("mount", "/");

        // Create the Compute Nodes
        std::vector<sg4::Host*> compute_nodes;
        for (unsigned int i = 0; i < num_hosts_; i++) {
            // Create a ComputeNode
            auto compute_node = zone->add_host("ServerlessComputeNode" + std::to_string(i + 1), "1Gf");
            compute_node->set_core_count(10);
            compute_node->set_property("ram", "64GB");
            auto compute_node_disk = compute_node->add_disk("hard_drive",
                                                            "100MBps",
                                                            "100MBps");
            compute_node_disk->set_property("size", "5000GiB");
            compute_node_disk->set_property("mount", "/");
            compute_nodes.push_back(compute_node);
        }

        // Create three network links and routes
        auto wide_area_link = zone->add_link("wide_area_link", "10MBps")->set_latency("20us");
        {
            sg4::LinkInRoute network_link_in_route{wide_area_link};
            zone->add_route(wms_host,
                            head_node,
                            {network_link_in_route});
        }

        for (unsigned int i = 0; i < num_hosts_; i++) {
            auto local_link = zone->add_link("local_link_" + std::to_string(i + 1),
                                             "100MBps")->set_latency("2us");
            {
                sg4::LinkInRoute network_link_in_route{local_link};
                zone->add_route(head_node,
                                compute_nodes[i],
                                {network_link_in_route});
            }
            {
                sg4::LinkInRoute network_link_in_route_1{wide_area_link};
                sg4::LinkInRoute network_link_in_route_2{local_link};
                zone->add_route(wms_host,
                                compute_nodes[i],
                                {network_link_in_route_1, network_link_in_route_2});
            }
        }

        zone->seal();
    }
};

/**
 * @brief The Simulator's main function
 *
 * @param argc: argument count
 * @param argv: argument array
 * @return 0 on success, non-zero otherwise
 */
int main(int argc, char** argv) {
    /*
     * Create a WRENCH simulation
     */
    const auto simulation = wrench::Simulation::createSimulation();

    /* Initialize the simulation, which may entail extracting WRENCH-specific and
     * Simgrid-specific command-line arguments that can modify general simulation behavior.
     * Two special command-line arguments are --help-wrench and --help-simgrid, which print
     * details about available command-line arguments. */
    simulation->init(&argc, argv);

    /* Parsing of the command-line arguments for this WRENCH simulation */
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] <<
            " <# function invocation> <# compute nodes> [--log=custom_controller.threshold=info]" << std::endl;
        exit(1);
    }
    unsigned int num_invocations = atoi(argv[1]);
    unsigned int num_compute_nodes = atoi(argv[2]);

    /* Using the basic First Come First Serve (FCFS) scheduler */
    auto scheduler = std::make_shared<wrench::FCFSServerlessScheduler>();

    /* Create the simulated platform */
    PlatformCreator platform_creator(num_compute_nodes);
    simulation->instantiatePlatform(platform_creator);

    /* Instantiate a storage service, on the user host */
    std::cerr << "Instantiating a SimpleStorageService on UserHost..." << std::endl;
    const auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
        "UserHost", {"/"}, {}, {}));

    /* Instantiate a serverless compute service */
    std::cerr << "Instantiating a serverless compute service on ServerlessHeadNode..." << std::endl;
    std::vector<std::string> compute_nodes;
    for (unsigned int i = 0; i < num_compute_nodes; i++) {
        std::string compute_node_name = "ServerlessComputeNode" + std::to_string(i + 1);
        compute_nodes.push_back(compute_node_name);
    }
    const auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
        "ServerlessHeadNode", "/", compute_nodes, scheduler, {}, {}));

    /* Instantiate an Execution Controller on UserHost */
    auto wms = simulation->add(
        new wrench::ServerlessExampleExecutionController(serverless_provider, storage_service, "UserHost",
                                                         num_invocations));

    /* Launch the simulation. This call only returns when the simulation is complete. */
    std::cerr << "Launching the Simulation..." << std::endl;
    try {
        simulation->launch();
    }
    catch (std::runtime_error& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    std::cerr << "Simulated execution time: " << wrench::Simulation::getCurrentSimulatedDate() << std::endl;

    return 0;
}
