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
#include <wrench/tools/wfcommons/WfCommonsWorkflowParser.h>

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
        std::cerr << "Usage: " << argv[0] << " <xml platform file> <workflow file> [--log=simple_wms.threshold=info]" << std::endl;
        exit(1);
    }

    /* The first argument is the platform description file, written in XML following the SimGrid-defined DTD */
    char *platform_file = argv[1];
    /* The second argument is the workflow description file, written in JSON using WfCommons's WfFormat format */
    char *workflow_file = argv[2];


    /* Reading and parsing the workflow description file to create a wrench::Workflow object */
    std::cerr << "Loading workflow..." << std::endl;
    std::shared_ptr<wrench::Workflow> workflow;
    if (ends_with(workflow_file,"json")) {
        workflow = wrench::WfCommonsWorkflowParser::createWorkflowFromJSON(workflow_file, "100Gf");
    } else {
        std::cerr << "Workflow file name must end with '.json'" << std::endl;
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
     * and adding it to the simulation.  A wrench::StorageService is an abstraction of a service on
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
     * The next argument to the constructor
     * shows how to configure particular simulated behaviors of the compute service via a property
     * list. In this case, we use the conservative_bf_core_level scheduling algorithm which implements
     * conservative backfilling at the core level (i.e., two jobs can shared a compute node by using different cores on it).
     * The last argument to the constructor makes it possible to specify various control message sizes.
     * In this example, one specifies that the message that will be send to the service to
     * terminate it will be 2048 bytes. See the documentation to find out all available
     * configurable properties for each kind of service.
     */
    std::shared_ptr<wrench::BatchComputeService> batch_compute_service;
#ifndef ENABLE_BATSCHED
    std::string scheduling_algorithm = "conservative_bf_core_level";
#else
    std::string scheduling_algorithm = "conservative_bf";
#endif
    try {
        batch_compute_service = simulation->add(new wrench::BatchComputeService(
        {"BatchHeadNode"}, {{"BatchNode1"}, {"BatchNode2"}}, "",
                {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, scheduling_algorithm}},
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

    /* Instantiate a WMS (which is an ExecutionController really), to be started on some host (wms_host), which is responsible
     * for executing the workflow.
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

    /* Enable some output time stamps */
    simulation->getOutput().enableWorkflowTaskTimestamps(true);

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

    simulation->getOutput().dumpWorkflowGraphJSON(workflow, "/tmp/workflow.json", true);

    /* Simulation results can be examined via simulation->getOutput(), which provides access to traces
     * of events. In the code below, go through some time-stamps and compute some statistics.
     */
    std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampTaskCompletion> *> trace;
    trace = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();
    std::cerr << "Number of entries in TaskCompletion trace: " << trace.size() << std::endl;
    unsigned long num_failed_tasks = 0;
    double computation_communication_ratio_average = 0.0;
    for (const auto &item : trace) {
        auto task = item->getContent()->getTask();
        if (task->getExecutionHistory().size() > 1) {
            num_failed_tasks++;
        }
        double io_time = task->getExecutionHistory().top().read_input_end - task->getExecutionHistory().top().read_input_start;
        io_time += task->getExecutionHistory().top().write_output_end - task->getExecutionHistory().top().write_output_start;
        double compute_time = task->getExecutionHistory().top().computation_end - task->getExecutionHistory().top().computation_start;
        computation_communication_ratio_average += compute_time / io_time;
    }
    computation_communication_ratio_average /= (double)(trace.size());

    std::cerr << "Number of tasks that failed at least once: " << num_failed_tasks << "\n";
    std::cerr << "Average computation time / communication+IO time ratio over all tasks: " << computation_communication_ratio_average << "\n";

    return 0;
}
