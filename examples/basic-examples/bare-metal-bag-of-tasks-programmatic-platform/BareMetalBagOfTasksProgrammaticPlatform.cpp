/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** This simulator simulates the execution of a bag-of-tasks workflow, that is, of a workflow
 ** in which each task has its own input file and its own output file, and tasks can be
 ** executed completely independently
 **
 **   InputFile #0 -> Task #0 -> OutputFile #1
 **   ...
 **   InputFile #n -> Task #n -> OutputFile #n
 **
 ** The compute platform comprises two hosts, WMSHost and ComputeHost. On WMSHost runs a simple storage
 ** service and a WMS (defined in class TwoTasksAtATimeWMS). On ComputeHost runs a bare metal
 ** compute service, that has access to the 10 cores of that host. Once the simulation is done,
 ** the completion time of each workflow task is printed.
 **
 ** Example invocation of the simulator for a 10-task workflow, with no logging:
 **    ./wrench-example-bare-metal-bag-of-tasks 10 ./two_hosts.xml
 **
 ** Example invocation of the simulator for a 10-task workflow, with only WMS logging:
 **    ./wrench-example-bare-metal-bag-of-tasks 10 ./two_hosts.xml --log=custom_wms.threshold=info
 **
 ** Example invocation of the simulator for a 6-task workflow with full logging:
 **    ./wrench-example-bare-metal-bag-of-tasks 6 ./two_hosts.xml --wrench-full-log
 **/


#include <iostream>
#include <wrench.h>

#include "TwoTasksAtATimeWMS.h" // WMS implementation

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
    PlatformCreator(double link_bw) : link_bw(link_bw) {}

    void operator()() const {
        create_platform(this->link_bw);
    }

private:
    double link_bw;

    void create_platform(double link_bw) const {
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
            sg4::LinkInRoute network_link_in_route{network_link};
            zone->add_route(compute_host->get_netpoint(),
                            wms_host->get_netpoint(),
                            nullptr,
                            nullptr,
                            {network_link_in_route});
        }
        {
            sg4::LinkInRoute network_link_in_route{loopback_WMSHost};
            zone->add_route(wms_host->get_netpoint(),
                            wms_host->get_netpoint(),
                            nullptr,
                            nullptr,
                            {network_link_in_route});
        }
        {
            sg4::LinkInRoute network_link_in_route{loopback_ComputeHost};
            zone->add_route(compute_host->get_netpoint(),
                            compute_host->get_netpoint(),
                            nullptr,
                            nullptr,
                            {network_link_in_route});
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
     * Declare a WRENCH simulation object
     */
    wrench::Simulation simulation;

    /* Initialize the simulation, which may entail extracting WRENCH-specific and
     * Simgrid-specific command-line arguments that can modify general simulation behavior.
     * Two special command-line arguments are --help-wrench and --help-simgrid, which print
     * details about available command-line arguments. */
    simulation.init(&argc, argv);

    /* Parsing of the command-line arguments for this WRENCH simulation */
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <an EVEN number of tasks> [--log=custom_wms.threshold=info]" << std::endl;
        exit(1);
    }

    /* Reading and parsing the platform description file, written in XML following the SimGrid-defined DTD,
     * to instantiate the simulated platform */
    std::cerr << "Instantiating simulated platform programmatically..." << std::endl;

    PlatformCreator platform_creator(100 * MBPS);
    simulation.instantiatePlatform(platform_creator);


    /* Parse the first command-line argument (number of tasks) */
    int num_tasks = 0;
    try {
        num_tasks = std::atoi(argv[1]);
        if (num_tasks  %  2)  {
            throw std::invalid_argument("Number of tasks should be even");
        }
    } catch (std::invalid_argument &e) {
        std::cerr << "Invalid number of tasks ("  << e.what() << ")\n";
        exit(1);
    }

    /* Declare a workflow */
    wrench::Workflow workflow;

    /* Initialize and seed a RNG */
    std::uniform_real_distribution<double> dist(100000000.0,10000000000.0);
    std::mt19937 rng(42);

    /* Add workflow tasks and files */
    for (int i=0; i < num_tasks; i++) {
        /* Create a task: random GFlop, 1 to 10 cores, 0.90 parallel efficiency, 10MB memory_manager_service footprint */
        auto task = workflow.addTask("task_" + std::to_string(i), dist(rng), 1, 10, 10000000);
        task->setParallelModel(wrench::ParallelModel::CONSTANTEFFICIENCY(0.9));
        task->addInputFile(workflow.addFile("input_" + std::to_string(i), 10000000));
        task->addOutputFile(workflow.addFile("output_" + std::to_string(i), 10000000));
    }

    /* Instantiate a storage service, and add it to the simulation.
     * A wrench::StorageService is an abstraction of a service on
     * which files can be written and read.  This particular storage service, which is an instance
     * of wrench::SimpleStorageService, is started on WMSHost in the
     * platform , which has an attached disk mounted at "/". The SimpleStorageService
     * is a basic storage service implementation provided by WRENCH.
     * Throughout the simulation execution, input/output files of workflow tasks will be located
     * in this storage service, and accessed remotely by the compute service. Note that the
     * storage service is configured to use a buffer size of 50M when transferring data over
     * the network (i.e., to pipeline disk reads/writes and network revs/sends). */
    std::cerr << "Instantiating a SimpleStorageService on WMSHost..." << std::endl;
    auto storage_service = simulation.add(new wrench::SimpleStorageService(
            "WMSHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "50000000"}}, {}));

    /* Instantiate a bare-metal compute service, and add it to the simulation.
     * A wrench::bare_metal_standard_jobs is an abstraction of a compute service that corresponds
     * to a software infrastructure that can execute tasks on hardware resources.
     * This particular service is started on ComputeHost and has no scratch storage space (mount point argument = "").
     * This means that tasks running on this service will access data only from remote storage services. */
    std::cerr << "Instantiating a bare_metal_standard_jobs on ComputeHost..." << std::endl;
    auto baremetal_service = simulation.add(new wrench::BareMetalComputeService(
            "ComputeHost", {"ComputeHost"}, "", {}, {}));

    /* Instantiate a WMS, to be stated on WMSHost, which is responsible
     * for executing the workflow. */

    auto wms = simulation.add(
            new wrench::TwoTasksAtATimeWMS({baremetal_service}, {storage_service}, "WMSHost"));

    /* Associate the workflow to the WMS */
    wms->addWorkflow(&workflow);

    /* Instantiate a file registry service to be started on WMSHost. This service is
     * essentially a replica catalog that stores <file , storage service> pairs so that
     * any service, in particular a WMS, can discover where workflow files are stored. */
    std::cerr << "Instantiating a FileRegistryService on WMSHost ..." << std::endl;
    auto file_registry_service = new wrench::FileRegistryService("WMSHost");
    simulation.add(file_registry_service);

    /* It is necessary to store, or "stage", input files that only input. The getInputFiles()
     * method of the Workflow class returns the set of all workflow files that are not generated
     * by workflow tasks, and thus are only input files. These files are then staged on the storage service. */
    std::cerr << "Staging task input files..." << std::endl;
    for (auto const &f : workflow.getInputFiles()) {
        simulation.stageFile(f, storage_service);
    }

    /* Launch the simulation. This call only returns when the simulation is complete. */
    std::cerr << "Launching the Simulation..." << std::endl;
    try {
        simulation.launch();
    } catch (std::runtime_error &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    std::cerr << "Simulation done!" << std::endl;

    /* Simulation results can be examined via simulation.output, which provides access to traces
     * of events. In the code below, we print the  retrieve the trace of all task completion events, print how
     * many such events there are, and print some information for the first such event. */
    auto trace = simulation.getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();
    for (auto const &item : trace) {
        std::cerr << "Task "  << item->getContent()->getTask()->getID() << " completed at time " << item->getDate()  << std::endl;
    }

    /* Dump it all to a JSON file */
    simulation.getOutput().dumpUnifiedJSON(&workflow, "/tmp/wrench.json");

    return 0;
}
