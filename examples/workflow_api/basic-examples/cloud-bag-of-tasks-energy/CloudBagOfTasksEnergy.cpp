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
 ** The compute platform comprises four hosts, WMSHost,  CloudProviderHost, CloudHost1, and
 **  CloudHost2. On WMSHost runs a simple storage
 ** service and a WMS (defined in class TwoTasksAtATimeCloudWMS). On CloudProviderHost runs a cloud
 ** service, that has access to two hosts: CloudHost1 and CloudHost2. Once the simulation is done,
 ** the completion time of each workflow task is printed.
 **
 ** This simulator simulates energy consumption. As such it must use a platform file that defines
 ** host pstates (such .xml as the file in this directory). It also uses an EnergyMeterService
 ** and outputs energy consumption measures. 
 **
 ** Example invocation of the simulator for a 10-task workflow, with no logging:
 **    ./wrench-example-cloud-bag-of-tasks 10 ./four_hosts.xml --wrench-energy-simulation
 **
 ** Example invocation of the simulator for a 10-task workflow, with only WMS logging:
 **    ./wrench-example-cloud-bag-of-tasks 10 ./four_hosts.xml --wrench-energy-simulation --log=custom_wms.threshold=info
 **
 ** Example invocation of the simulator for a 6-task workflow with full logging:
 **    ./wrench-example-cloud-bag-of-tasks 6 ./four_hosts.xml --wrench-energy-simulation  --wrench-full-log
 **/


#include <iostream>
#include <wrench.h>

#include "TwoTasksAtATimeCloudWMS.h"// WMS implementation

/**
 * @brief The Simulator's main function
 *
 * @param argc: argument count
 * @param argv: argument array
 * @return 0 on success, non-zero otherwise
 */
int main(int argc, char **argv) {

    /* Create a WRENCH simulation object */
    auto simulation = wrench::Simulation::createSimulation();

    /* Initialize the simulation, which may entail extracting WRENCH-specific and
     * Simgrid-specific command-line arguments that can modify general simulation behavior.
     * Two special command-line arguments are --help-wrench and --help-simgrid, which print
     * details about available command-line arguments. */
    simulation->init(&argc, argv);

    /* Parsing of the command-line arguments for this WRENCH simulation */
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <an EVEN number of tasks> <xml platform file> --wrench-energy-simulation [--log=custom_wms.threshold=info]" << std::endl;
        exit(1);
    }

    /* Reading and parsing the platform description file, written in XML following the SimGrid-defined DTD,
     * to instantiate the simulated platform */
    std::cerr << "Instantiating simulated platform..." << std::endl;
    simulation->instantiatePlatform(argv[2]);

    /* Parse the first command-line argument (number of tasks) */
    int num_tasks = 0;
    try {
        num_tasks = std::atoi(argv[1]);
        if (num_tasks % 2) {
            throw std::invalid_argument("Number of tasks should be even");
        }
    } catch (std::invalid_argument &e) {
        std::cerr << "Invalid number of tasks (" << e.what() << ")\n";
        exit(1);
    }

    /* Declare a workflow */
    auto workflow = wrench::Workflow::createWorkflow();

    /* Initialize and seed a RNG */
    std::uniform_real_distribution<double> dist(1000000000000.0, 100000000000000.0);
    std::mt19937 rng(42);

    /* Add workflow tasks and files */
    for (int i = 0; i < num_tasks; i++) {
        /* Create a task: random GFlop, 1 to 10 cores, 0.90 parallel efficiency, 10MB memory_manager_service footprint */
        auto task = workflow->addTask("task_" + std::to_string(i), dist(rng), 1, 10, 1000);
        task->setParallelModel(wrench::ParallelModel::CONSTANTEFFICIENCY(0.9));
        task->addInputFile(workflow->addFile("input_" + std::to_string(i), 10000000));
        task->addOutputFile(workflow->addFile("output_" + std::to_string(i), 10000000));
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
    auto storage_service = simulation->add(new wrench::SimpleStorageService(
            "WMSHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "50000000"}}, {}));

    /* Instantiate a cloud compute service, and add it to the simulation.
     * A wrench::CloudComputeService is an abstraction of a compute service that corresponds
     * to a cloud that responds to VM creating requests, and each VM exposes a "bare-metal" compute service.
     * This particular service is started on CloudProviderHost, uses CloudHost1 and CloudHost2
     * as hardware resources, and has no scratch storage space (mount point argument = "").
     * This means that tasks running on this service will access data only from remote storage services. */
    std::cerr << "Instantiating a CloudComputeService on CloudProviderHost..." << std::endl;
    std::vector<std::string> cloud_hosts = {"CloudHost1", "CloudHost2"};
    auto cloud_service = simulation->add(new wrench::CloudComputeService(
            "CloudProviderHost", cloud_hosts, "", {}, {}));

    /* Instantiate a WMS, to be stated on WMSHost, which is responsible
     * for executing the workflow-> */
    auto wms = simulation->add(
            new wrench::TwoTasksAtATimeCloudWMS(workflow, cloud_service, storage_service, "WMSHost"));

    /* Instantiate a file registry service to be started on WMSHost. This service is
     * essentially a replica catalog that stores <file , storage service> pairs so that
     * any service, in particular a WMS, can discover where workflow files are stored. */
    std::cerr << "Instantiating a FileRegistryService on WMSHost ..." << std::endl;
    auto file_registry_service = new wrench::FileRegistryService("WMSHost");
    simulation->add(file_registry_service);

    /* Instantiate an energy meter service to  be started on WMSHost. This service
     * simply takes energy consumption measurements every minute at host CloudHost1. */
    std::cerr << "Instantiating an EnergyMeterService on WMSHost that monitors CloudHost1 every minute ..." << std::endl;
    auto energy_meter_service = simulation->add(new wrench::EnergyMeterService("WMSHost", {"CloudHost1"}, 60));

    /* It is necessary to store, or "stage", input files that only input. The getInputFiles()
     * method of the Workflow class returns the set of all workflow files that are not generated
     * by workflow tasks, and thus are only input files. These files are then staged on the storage service. */
    std::cerr << "Staging task input files..." << std::endl;
    for (auto const &f: workflow->getInputFiles()) {
        simulation->stageFile(f, storage_service);
    }

    /* Launch the simulation. This call only returns when the simulation is complete. */
    std::cerr << "Launching the Simulation..." << std::endl;
    try {
        simulation->launch();
    } catch (std::runtime_error &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    std::cerr << "Simulation done!" << std::endl;

    /* Simulation results can be examined via simulation->getOutput(), which provides access to traces
     * of events. In the code below, we print the  retrieve the trace of all task completion events, print how
     * many such events there are, and print some information for the first such event. */
    auto trace = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();
    for (auto const &item: trace) {
        std::cerr << "Task " << item->getContent()->getTask()->getID() << " completed at time " << item->getDate() << std::endl;
    }

    /* The total energy consumed by hosts is readily available */
    std::cerr << "Total energy consumed at host WMSHost: " << simulation->getEnergyConsumed("WMSHost") << "\n";
    std::cerr << "Total energy consumed at host CloudProviderHost: " << simulation->getEnergyConsumed("CloudProviderHost") << "\n";
    std::cerr << "Total energy consumed at host CloudHost1: " << simulation->getEnergyConsumed("CloudHost1") << "\n";
    std::cerr << "Total energy consumed at host CloudHost2: " << simulation->getEnergyConsumed("CloudHost2") << "\n";

    /* The EnergyMeter service has generated energy consumption timestamps! */
    auto energy_trace = simulation->getOutput().getTrace<wrench::SimulationTimestampEnergyConsumption>();
    std::cerr << "Timeline of the energy consumption at host CloudHost1 (as measured by the EnergyMeter service):\n";
    for (auto const &item: energy_trace) {
        std::cerr << "  - " << item->getDate() << ": " << std::round(item->getContent()->getConsumption()) << " Joules\n";
    }

    /* Dump it all to a JSON file */
    simulation->getOutput().dumpUnifiedJSON(workflow, "/tmp/wrench.json", false, true, false, true, false, false, false);

    return 0;
}
