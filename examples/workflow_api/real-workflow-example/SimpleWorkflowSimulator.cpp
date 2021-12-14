/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <iostream>
#include <wrench.h>

#include "SimpleWMS.h"
#include <wrench/tools/pegasus/PegasusWorkflowParser.h>

static bool ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}

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
        std::cerr << "Usage: " << argv[0] << " <xml platform file> <workflow file>" << std::endl;
        exit(1);
    }

    /* The first argument is the platform description file, written in XML following the SimGrid-defined DTD */
    char *platform_file = argv[1];
    /* The second argument is the workflow description file, written in XML using the DAX DTD */
    char *workflow_file = argv[2];


    /* Reading and parsing the workflow description file to create a wrench::Workflow object */
    std::cerr << "Loading workflow->.." << std::endl;
    std::shared_ptr<wrench::Workflow> workflow;
    if (ends_with(workflow_file, "dax")) {
        workflow = wrench::PegasusWorkflowParser::createWorkflowFromDAX(workflow_file, "1000Gf");
    } else if (ends_with(workflow_file,"json")) {
        workflow = wrench::PegasusWorkflowParser::createWorkflowFromJSON(workflow_file, "1000Gf");
    } else {
        std::cerr << "Workflow file name must end with '.dax' or '.json'" << std::endl;
        exit(1);
    }
    std::cerr << "The workflow has " << workflow->getNumberOfTasks() << " tasks " << std::endl;
    std::cerr.flush();

    /* Reading and parsing the platform description file to instantiate a simulated platform */
    std::cerr << "Instantiating SimGrid platform..." << std::endl;
    simulation->instantiatePlatform(platform_file);

    /* Get a vector of all the hosts in the simulated platform */
    std::vector<std::string> hostname_list = wrench::Simulation::getHostnameList();

    /* Create a list of storage services that will be used by the WMS */
    std::set<std::shared_ptr<wrench::StorageService>> storage_services;

    /* Instantiate a storage service, to be started on some host in the simulated platform,
     * and adding it to the simulation->  A wrench::StorageService is an abstraction of a service on
     * which files can be written and read.  This particular storage service, which is an instance
     * of wrench::SimpleStorageService, is started on WMSHost in the
     * platform (platform/batch_platform.xml), which has an attached disk called large_disk. The SimpleStorageService
     * is a barebone storage service implementation provided by WRENCH.
     * Throughout the simulation execution, input/output files of workflow tasks will be located
     * in this storage service.
     */
    std::cerr << "Instantiating a SimpleStorageService on WMSHost " << std::endl;
    auto storage_service = simulation->add(new wrench::SimpleStorageService({"WMSHost"}, {"/"}));
    storage_services.insert(storage_service);


    /* Create a list of compute services that will be used by the WMS */
    std::set<std::shared_ptr<wrench::ComputeService>> compute_services;

    /* Instantiate and add to the simulation a batch_standard_and_pilot_jobs service, to be started on some host in the simulation platform.
     * A batch_standard_and_pilot_jobs service is an abstraction of a compute service that corresponds to
     * batch_standard_and_pilot_jobs-scheduled platforms in which jobs are submitted to a queue and dispatched
     * to compute nodes according to various scheduling algorithms.
     * In this example, this particular batch_standard_and_pilot_jobs service has no scratch storage space (mount point = "").
     * The last argument to the constructor
     * shows how to configure particular simulated behaviors of the compute service via a property
     * list. In this example, one specifies that the message that will be send to the service to
     * terminate it will be 2048 bytes. See the documentation to find out all available
     * configurable properties for each kind of service.
     */
    std::shared_ptr<wrench::BatchComputeService> batch_compute_service;
    try {
        batch_compute_service = simulation->add(new wrench::BatchComputeService(
        {"BatchHeadNode"}, {{"BatchNode1"}, {"BatchNode2"}}, "",
                {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "conservative_bf"}},
                {{wrench::BatchComputeServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 2048}}));
    } catch (std::invalid_argument &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::exit(1);}

    /* Instantiate and add to the simulation a cloud service, to be started on some host in the simulation platform.
     * A cloud service is an abstraction of a compute service that corresponds to a
     * Cloud platform that provides access to virtualized compute resources.
     * In this example, this particular cloud service has no scratch storage space (mount point = "").
     * The last argument to the constructor
     * shows how to configure particular simulated behaviors of the compute service via a property
     * list. In this example, one specified that the message that will be send to the service to
     * terminate it will by 1024 bytes. See the documentation to find out all available
     * configurable properties for each kind of service.
     */
    std::shared_ptr<wrench::CloudComputeService> cloud_compute_service;
    try {
        cloud_compute_service = simulation->add(new wrench::CloudComputeService(
                {"CloudHeadNode"}, {{"CloudNode1"}, {"CloudNode2"}}, "", {},
                {{wrench::CloudComputeServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 1024}}));
    } catch (std::invalid_argument &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::exit(1);
    }

    /* Instantiate a WMS, to be started on some host (wms_host), which is responsible
     * for executing the workflow, and uses a scheduler (BatchStandardJobScheduler). That scheduler
     * is instantiated with the batch_standard_and_pilot_jobs service, the list of hosts available for running
     * tasks, and also provided a pointer to the simulation object.
     *
     * The WMS implementation is in SimpleWMS.[cpp|h].
     */
    std::cerr << "Instantiating a WMS on WMSHost..." << std::endl;
    auto wms = simulation->add(
            new wrench::SimpleWMS(workflow, batch_compute_service,
                                  cloud_compute_service, storage_service, {"WMSHost"}));

    /* Instantiate a file registry service to be started on some host. This service is
     * essentially a replica catalog that stores <file , storage service> pairs so that
     * any service, in particular a WMS, can discover where workflow files are stored.
     */
    std::string file_registry_service_host = hostname_list[(hostname_list.size() > 2) ? 1 : 0];
    std::cerr << "Instantiating a FileRegistryService on " << file_registry_service_host << "..." << std::endl;
    auto file_registry_service =
            simulation->add(new wrench::FileRegistryService(file_registry_service_host));

    /* It is necessary to store, or "stage", input files for the first task1(s) of the workflow on some storage
     * service, so that workflow execution can be initiated. The getInputFiles() method of the Workflow class
     * returns the set of all workflow files that are not generated by workflow tasks, and thus are only input files.
     * These files are then staged on the storage service.
     */
    std::cerr << "Staging input files..." << std::endl;
    for (auto const &f : workflow->getInputFiles()) {
        try {
            simulation->stageFile(f, storage_service);
        } catch (std::runtime_error &e) {
            std::cerr << "Exception: " << e.what() << std::endl;
            return 0;
        }
    }

    /* Launch the simulation-> This call only returns when the simulation is complete. */
    std::cerr << "Launching the Simulation..." << std::endl;
    try {
        simulation->launch();
    } catch (std::runtime_error &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 0;
    }
    std::cerr << "Simulation done!" << std::endl;

    /* Simulation results can be examined via simulation->output, which provides access to traces
     * of events. In the code below, we retrieve the trace of all task1 completion events, print how
     * many such events there are, and print some information for the first such event.
     */
    std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampTaskCompletion> *> trace;
    trace = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();
    std::cerr << "Number of entries in TaskCompletion trace: " << trace.size() << std::endl;
    std::cerr << "Task in first trace entry: " << trace[0]->getContent()->getTask()->getID() << std::endl;

    return 0;
}
