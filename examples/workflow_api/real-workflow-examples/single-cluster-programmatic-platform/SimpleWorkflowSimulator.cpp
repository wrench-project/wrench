/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <iostream>
#include "wrench.h"

#include "SimpleWMS.h"
#include "wrench/tools/wfcommons/WfCommonsWorkflowParser.h"
#include <iostream>
#include <fstream>

#define MBPS (1000 * 1000)

namespace sg4 = simgrid::s4u;

/**
 * @brief Function to instantiate a simulated platform, instead of
 * loading it from an XML file. This function directly uses SimGrid's s4u API
 * (see the SimGrid documentation). This function creates a platform that's
 * identical to that described in the file two_hosts.xml located in this directory.
 */

class PlatformCreator {

public:
    PlatformCreator(unsigned long num_compute_hosts) : num_compute_hosts(num_compute_hosts) {}

    void operator()() const {
        create_platform();
    }

private:
    unsigned long num_compute_hosts;

    void create_platform() const {
        // Create the top-level zone
        auto zone = sg4::create_full_zone("AS0");

        // Create the WMSHost host with its disk
        auto wms_host = zone->create_host("WMSHost", "10Gf");
        wms_host->set_core_count(1);
        auto wms_host_disk = wms_host->create_disk("hard_drive",
                                                   "100MBps",
                                                   "100MBps");
        wms_host_disk->set_property("size", "5000GiB");
        wms_host_disk->set_property("mount", "/");

        // Create a single network link that abstracts the wide-area network
        auto network_link = zone->create_link("network_link", 100 * MBPS)->set_latency("20us");

        // Create the compute hosts and routes to them (could be done as a single cluster)
        for (int i=0; i < num_compute_hosts; i++) {
            auto compute_host = zone->create_host("ComputeHost_" + std::to_string(i), "1Gf");
            compute_host->set_core_count(1);
            sg4::LinkInRoute network_link_in_route{network_link};
            zone->add_route(compute_host,
                            wms_host,
                            {network_link_in_route});
        }

        zone->seal();
    }
};

/**
 * @brief An example that demonstrate how to run a simulation of a simple Workflow
 *        Management System (WMS) (implemented in SimpleWMS.[cpp|h]).
 *
 * @param argc: argument count
 * @param argv: argument array
 * @return 0 if the simulation has successfully completed
 */
int main(int argc, char **argv) {

    /*
     * Declaration of the top-level WRENCH simulation object
     */
    auto simulation = wrench::Simulation::createSimulation();

    /*
     * Initialization of the simulation, which may entail extracting WRENCH-specific and
     * Simgrid-specific command-line arguments that can modify general simulation behavior.
     * Two special command-line arguments are --help-wrench and --help-simgrid, which print
     * details about available command-line arguments.
     */
    simulation->init(&argc, argv);

    /*
     * Parsing of the command-line arguments for this WRENCH simulation
     */
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <# compute hosts> <workflow file> [--log=simple_wms.threshold=info]" << std::endl;
        exit(1);
    }

    /* The first argument is the workflow description file, written in JSON using WfCommons's WfFormat format */
    char *workflow_file = argv[2];

    /* The second argument is the number of compute hosts to simulate */
    int num_compute_hosts;
    try {
        num_compute_hosts = std::atoi(argv[1]);
    } catch (std::invalid_argument &e) {
        std::cerr << "Invalid number of compute hosts\n";
        exit(1);
    }

    /* Reading and parsing the workflow description file to create a wrench::Workflow object */
    std::cerr << "Loading workflow..." << std::endl;
    std::shared_ptr<wrench::Workflow> workflow;
    workflow = wrench::WfCommonsWorkflowParser::createWorkflowFromJSON(workflow_file, "100Gf", true);
    std::cerr << "The workflow has " << workflow->getNumberOfTasks() << " tasks " << std::endl;
    std::cerr.flush();

    /* Reading and parsing the platform description file to instantiate a simulated platform */
    std::cerr << "Instantiating SimGrid platform programmatically ..." << std::endl;
    PlatformCreator platform_creator(num_compute_hosts);
    simulation->instantiatePlatform(platform_creator);

    /* Get a vector of all the hosts in the simulated platform */
    std::vector<std::string> hostname_list = wrench::Simulation::getHostnameList();

    /* Instantiate a storage service, to be started on the WMShost */
    std::cerr << "Instantiating a SimpleStorageService on WMSHost " << std::endl;
    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService({"WMSHost"}, {"/"}));

    /* Create a list of compute services */
    std::set<std::shared_ptr<wrench::BareMetalComputeService>> compute_services;

    /* Create a bare-metal compute service on each compute host in the platform */
    for (auto const &hostname : wrench::Simulation::getHostnameList()) {
        if (hostname != "WMSHost") {
            auto cs = simulation->add(new wrench::BareMetalComputeService({hostname}, {hostname}, "", {}, {}));
            compute_services.insert(cs);
        }
    }

    /* Instantiate a WMS (which is an ExecutionController really), to be started on some host (wms_host), which is responsible
     * for executing the workflow.
     *
     * The WMS implementation is in SimpleWMS.[cpp|h].
     */
    std::cerr << "Instantiating a WMS on WMSHost..." << std::endl;
    auto wms = simulation->add(
            new wrench::SimpleWMS(workflow, compute_services, storage_service, {"WMSHost"}));

    /* It is necessary to store, or "stage", input files for the first task(s) of the workflow on some storage
     * service, so that workflow execution can be initiated. The getInputFiles() method of the Workflow class
     * returns the set of all files that are not generated by workflow tasks, and thus are only input files.
     * These files are then staged on the storage service.
     */
    std::cerr << "Staging input files..." << std::endl;
    for (auto const &f: workflow->getInputFiles()) {
        try {
            storage_service->createFile(f);
        } catch (std::runtime_error &e) {
            std::cerr << "Exception: " << e.what() << std::endl;
            return 0;
        }
    }

    /* Launch the simulation. This call only returns when the simulation is complete. */
    std::cerr << "Launching the Simulation..." << std::endl;
    try {
        simulation->launch();
    } catch (std::runtime_error &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 0;
    }
    std::cerr << "Simulation done!" << std::endl;
    std::cerr << "Workflow completed at time: " << workflow->getCompletionDate() << std::endl;

    return 0;
}
