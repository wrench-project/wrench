/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <iostream>
#include <wrench.h>

#include "SimpleWMS.h"
#include "scheduler/CloudStandardJobScheduler.h"
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
    auto simulation = wrench::Simulation::createSimulation();;

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
     * of wrench::SimpleStorageService, is started on Host3 in the
     * platform (platform/batch_platform.xml), which has an attached disk called large_disk. The SimpleStorageService
     * is a barebone storage service implementation provided by WRENCH.
     * Throughout the simulation execution, input/output files of workflow tasks will be located
     * in this storage service.
     */
    std::string storage_host = "Fafard";
    std::cerr << "Instantiating a SimpleStorageService on " << storage_host << "..." << std::endl;
    auto storage_service = simulation->add(new wrench::SimpleStorageService(storage_host, {"/"}));
    storage_services.insert(storage_service);

    /* Construct a list of hosts (in the example only one host) on which the
     * cloud service will be able to run tasks
     */
    std::string executor_host = "Tremblay";
    std::vector<std::string> execution_hosts = {executor_host};

    /* Create a list of compute services that will be used by the WMS */
    std::set<std::shared_ptr<wrench::ComputeService>> compute_services;

    /* Instantiate a cloud service, to be started on some host in the simulation platform.
     * A cloud service is an abstraction of a compute service that corresponds to a
     * Cloud platform that provides access to virtualized compute resources.
     * In this example, this particular cloud service has no scratch storage space (mount point = "").
     * The last argument to the constructor
     * shows how to configure particular simulated behaviors of the compute service via a property
     * list. In this example, one specified that the message that will be send to the service to
     * terminate it will by 1024 bytes. See the documentation to find out all available
     * configurable properties for each kind of service.
     */
    std::string wms_host = "Jupiter";

    /* Add the cloud service to the simulation, catching a possible exception */
    try {
        auto cloud_service = simulation->add(new wrench::CloudComputeService(
                wms_host, execution_hosts, "", {},
                {{wrench::CloudComputeServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 1024}}));
        compute_services.insert(cloud_service);
    } catch (std::invalid_argument &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::exit(1);
    }



    /* Instantiate a WMS, to be started on some host (wms_host), which is responsible
     * for executing the workflow, and uses a scheduler (CloudStandardJobScheduler). That scheduler
     * is instantiated with the cloud service, the list of hosts available for running
     * tasks, and also provided a pointer to the simulation object.
     *
     * The WMS implementation is in SimpleWMS.[cpp|h].
     */
    auto wms = simulation->add(
            new wrench::SimpleWMS(workflow, std::unique_ptr<wrench::CloudStandardJobScheduler>(
                    new wrench::CloudStandardJobScheduler(storage_service)),
                                  nullptr, compute_services, storage_services, wms_host));

    /* Instantiate a file registry service to be started on some host. This service is
     * essentially a replica catalog that stores <file , storage service> pairs so that
     * any service, in particular a WMS, can discover where workflow files are stored.
     */
    std::string file_registry_service_host = hostname_list[(hostname_list.size() > 2) ? 1 : 0];
    std::cerr << "Instantiating a FileRegistryService on " << file_registry_service_host << "..." << std::endl;
    wrench::FileRegistryService *file_registry_service =
            new wrench::FileRegistryService(file_registry_service_host);
    simulation->add(file_registry_service);

    // TRYING NETWORK PROXIMITY SERVICE WITH VIVALDI....
    std::vector<std::string> hostname_list_copy(hostname_list);
    std::string hostname_copy(hostname_list[0]);
    auto NPS = new wrench::NetworkProximityService(hostname_copy, hostname_list_copy, {
            {wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE,                 "alltoall"},
            {wrench::NetworkProximityServiceProperty::NETWORK_DAEMON_COMMUNICATION_COVERAGE,          "1.0"},
            {wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_MEASUREMENT_PERIOD,           "600"},
            {wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_MEASUREMENT_PERIOD_MAX_NOISE, "10"}});

    simulation->add(NPS);
    /* It is necessary to store, or "stage", input files for the first task(s) of the workflow on some storage
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
     * of events. In the code below, we retrieve the trace of all task completion events, print how
     * many such events there are, and print some information for the first such event.
     */
    std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampTaskCompletion> *> trace;
    trace = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();
    std::cerr << "Number of entries in TaskCompletion trace: " << trace.size() << std::endl;
    std::cerr << "Task in first trace entry: " << trace[0]->getContent()->getTask()->getID() << std::endl;

    return 0;
}
