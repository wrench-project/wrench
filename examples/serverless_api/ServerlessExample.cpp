/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** TODO: This simulator simulates the execution of a bag-of-actions application, that is, a bunch
 ** of independent compute actions, each with its own input file and its own output file. Actions can be
 ** executed completely independently:
 **
 **   InputFile #0 -> Action #0 -> OutputFile #1
 **   ...
 **   InputFile #n -> Action #n -> OutputFile #n
 **
 ** The compute platform comprises two hosts, ControllerHost and ComputeHost. On ControllerHost runs a simple storage
 ** service and an execution controller (defined in class TwoActionsAtATimeExecutionController). On ComputeHost runs a bare metal
 ** compute service, that has access to the 10 cores of that host.
 **
 ** Example invocation of the simulator for a 10-compute-action workload, with no logging:
 **    ./wrench-example-bare-metal-bag-of-actions 10 ./two_hosts.xml
 **
 ** Example invocation of the simulator for a 10-compute-action workload, with only execution controller logging:
 **    ./wrench-example-bare-metal-bag-of-actions 10 ./two_hosts.xml --log=custom_controller.threshold=info
 **
 ** Example invocation of the simulator for a 6-compute-action workload with full logging:
 **    ./wrench-example-bare-metal-bag-of-actions 6 ./two_hosts.xml --wrench-full-log
 **/


#include <iostream>
#include <wrench.h>

#include "ServerlessExampleExecutionController.h"
#include "wrench/services/compute/serverless/schedulers/RandomServerlessScheduler.h"
#include "wrench/services/compute/serverless/schedulers/FCFSServerlessScheduler.h"
#include "wrench/services/compute/serverless/schedulers/WorkloadBalancingServerlessScheduler.h"

namespace sg4 = simgrid::s4u;

class PlatformCreator {

public:
    explicit PlatformCreator(unsigned int num_hosts) : num_hosts(num_hosts) {}

    void operator()() const {
        create_platform(num_hosts);
    }

private:
    unsigned int num_hosts;

    void create_platform(unsigned int num_hosts) const {
        // Create the top-level zone
        auto zone = sg4::create_full_zone("AS0");
        // Create the WMSHost host with its disk
        auto wms_host = zone->create_host("UserHost", "10Gf");
        wms_host->set_core_count(1);
        auto wms_host_disk = wms_host->create_disk("hard_drive",
                                                   "100MBps",
                                                   "100MBps");
        wms_host_disk->set_property("size", "5000GiB");
        wms_host_disk->set_property("mount", "/");

        // Create the ServerlessHeadNode host with its disk
        auto head_node = zone->create_host("ServerlessHeadNode", "10Gf");
        head_node->set_core_count(1);
        auto head_node_disk = head_node->create_disk("hard_drive",
                                                   "100MBps",
                                                   "100MBps");
        head_node_disk->set_property("size", "5000GiB");
        head_node_disk->set_property("mount", "/");

        // Create the Compute Nodes
        std::vector<sg4::Host *> compute_nodes;
        for (unsigned int i=0; i < num_hosts; i++) {
            // Create a ComputeNode
            auto compute_node = zone->create_host("ServerlessComputeNode" + std::to_string(i+1), "1Gf");
            compute_node->set_core_count(10);
            compute_node->set_property("ram", "64GB");
            auto compute_node_disk = compute_node->create_disk("hard_drive",
                                                   "100MBps",
                                                   "100MBps");
            compute_node_disk->set_property("size", "5000GiB");
            compute_node_disk->set_property("mount", "/");
            compute_nodes.push_back(compute_node);
        }

        // Create three network links and routes
        auto wide_area_link = zone->create_link("wide_area_link", "10MBps")->set_latency("20us");
        // auto loopback_WMSHost = zone->create_link("loopback_WMSHost", "1000EBps")->set_latency("0us");
        // auto loopback_ComputeHost = zone->create_link("loopback_ComputeHost", "1000EBps")->set_latency("0us");
        {
            sg4::LinkInRoute network_link_in_route{wide_area_link};
            zone->add_route(wms_host,
                            head_node,
                            {network_link_in_route});
        }

        for (unsigned int i=0; i < num_hosts; i++) {
            auto local_link = zone->create_link("local_link_" + std::to_string(i+1),
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
int main(int argc, char **argv) {

    /*
     * Create a WRENCH simulation object
     */
    const auto simulation = wrench::Simulation::createSimulation();

    /* Initialize the simulation, which may entail extracting WRENCH-specific and
     * Simgrid-specific command-line arguments that can modify general simulation behavior.
     * Two special command-line arguments are --help-wrench and --help-simgrid, which print
     * details about available command-line arguments. */
    simulation->init(&argc, argv);

    /* Parsing of the command-line arguments for this WRENCH simulation */
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <# compute hosts> [--log=custom_controller.threshold=info]" << std::endl;
        exit(1);
    }

    std::string scheduler_type = "random";
    unsigned int num_invocations = 20;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg.rfind("--scheduler=", 0) == 0) {
            scheduler_type = arg.substr(12);
            cout << "Scheduler type: " << scheduler_type << endl;
        }
        else if (arg.rfind("--invocations=", 0) == 0) {
            num_invocations = std::stoul(arg.substr(14));
        }
    }
    
    std::shared_ptr<wrench::ServerlessScheduler> sched;
    if (scheduler_type == "random") {
        sched = std::make_shared<wrench::RandomServerlessScheduler>();
    } else if (scheduler_type == "fcfs") {
        sched = std::make_shared<wrench::FCFSServerlessScheduler>();
    } else if (scheduler_type == "balance") {
        sched = std::make_shared<wrench::WorkloadBalancingServerlessScheduler>();
    } else {
        std::cerr << "Unknown scheduler: " << scheduler_type << "\n";
        return 1;
    }


    /* Reading and parsing the platform description file, written in XML following the SimGrid-defined DTD,
     * to instantiate the simulated platform */
    std::cerr << "Instantiating simulated platform..." << std::endl;
    // simulation->instantiatePlatform(argv[1]);
    PlatformCreator platform_creator(atoi(argv[1]));
    simulation->instantiatePlatform(platform_creator);

    /* Instantiate a storage service, and add it to the simulation.
     * A wrench::StorageService is an abstraction of a service on
     * which files can be written and read.  This particular storage service, which is an instance
     * of wrench::SimpleStorageService, is started on UserHost in the
     * platform , which has an attached disk mounted at "/". The SimpleStorageService
     * is a basic storage service implementation provided by WRENCH.
     * Throughout the simulation execution, data files will be located
     * in this storage service, and accessed remotely by the compute service. Note that the
     * storage service is configured to use a buffer size of 50MB when transferring data over
     * the network (i.e., to pipeline disk reads/writes and network revs/sends). */
    std::cerr << "Instantiating a SimpleStorageService on UserHost..." << std::endl;
    const auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
            "UserHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "50MB"}}, {}));

    /* Instantiate a serverless compute service */
    std::cerr << "Instantiating a serverless compute service on ServerlessHeadNode..." << std::endl;
    // const std::vector<std::string> compute_nodes = {"ServerlessComputeNode1", "ServerlessComputeNode2"};
    std::vector<std::string> compute_nodes;
    for (unsigned int i=0; i < atoi(argv[1]); i++) {
        std::string compute_node_name = "ServerlessComputeNode" + std::to_string(i+1);
        compute_nodes.push_back(compute_node_name);
    }
    const auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
            "ServerlessHeadNode", compute_nodes, "/", sched, {}, {}));

    /* Instantiate an Execution controller, to be stated on UserHost, which is responsible
     * for executing the workflow-> */
    auto wms = simulation->add(
            new wrench::ServerlessExampleExecutionController(serverless_provider, storage_service, "UserHost", num_invocations));

    /* Launch the simulation. This call only returns when the simulation is complete. */
    std::cerr << "Launching the Simulation..." << std::endl;
    try {
        simulation->launch();
    } catch (std::runtime_error &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    std::cerr << "Simulated execution time: " << wrench::Simulation::getCurrentSimulatedDate() << std::endl;

    return 0;
}
