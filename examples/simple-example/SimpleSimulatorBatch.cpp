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
#include "scheduler/BatchStandardJobScheduler.h"

int main(int argc, char **argv) {

  /*
   * Declaration of the top-level WRENCH simulation object
   */
  wrench::Simulation simulation;

  /*
   * Initialization of the simulation, which may entail extracting WRENCH-specific
   * command-line arguments that can modify general simulation behavior (this includes
   * the many optional SimGrid command-line arguments as described in the SimGrid simulation)
   */
  simulation.init(&argc, argv);

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
  std::cerr << "Loading workflow..." << std::endl;
  wrench::Workflow workflow;
  workflow.loadFromDAX(workflow_file, "1000Gf");
  std::cerr << "The workflow has " << workflow.getNumberOfTasks() << " tasks " << std::endl;
  std::cerr.flush();

  /* Reading and parsing the platform description file to instantiate a simulated platform */
  std::cerr << "Instantiating SimGrid platform..." << std::endl;
  simulation.instantiatePlatform(platform_file);

  /* Get a vector of all the hosts in the simulated platform */
  std::vector<std::string> hostname_list = simulation.getHostnameList();

  /* Instantiate a storage service, to be stated on some host in the simulated platform,
   * and adding it to the simulation.  A wrench::StorageService is an abstraction of a service on
   * which files can be written and read.  This particular storage service  has a capacity
   * of 10,000,000,000,000 bytes, and is a SimpleStorageService instance. The SimpleStorageService
   * is a barebone storage service implementation provided by WRENCH.
   * Throughout the simulation execution, input/output files of workflow tasks will be located
   * in this storage service.
   */
  std::string storage_host = hostname_list[(hostname_list.size() > 2) ? 2 : 1];
  std::cerr << "Instantiating a SimpleStorageService on " << storage_host << "..." << std::endl;
  wrench::StorageService *storage_service = simulation.add(
          new wrench::SimpleStorageService(storage_host, 10000000000000.0));

  std::string wms_host = hostname_list[0];

  /* Instantiate a batch service, to be started on some host in the simulation platform.
   * A batch service is an abstraction of a compute service that corresponds to
   * batch-scheduled platforms in which jobs are submitted to a queue and dispatched
   * to compute nodes according to various scheduling algorithms.
   * In this example, this particular batch service supports both standard jobs and pilot jobs.
   * Unless otherwise specified, tasks running on the service will read/write workflow files
   * from the storage service instantiated above. Finally, the last argument to the constructor
   * shows how to configure particular simulated behaviors of the compute service via a property
   * list. In this example, one specifies that the message that will be send to the service to
   * terminate it will be 2048 bytes. See the documentation to find out all available
   * configurable properties for each kind of service.
   */
  wrench::ComputeService *batch_service = new wrench::BatchService(
          wms_host, true, true, hostname_list, storage_service,
          {{wrench::BatchServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD, "2048"}});

  /* Add the batch service to the simulation, catching a possible exception */
  try {
    simulation.add(batch_service);

  } catch (std::invalid_argument &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    std::exit(1);
  }

  /* Create a list of compute services that will be used by the WMS */
  std::set<wrench::ComputeService *> compute_services;
  compute_services.insert(batch_service);

  /* Create a list of storage services that will be used by the WMS */
  std::set<wrench::StorageService *> storage_services;
  storage_services.insert(storage_service);

  /* Instantiate a WMS, to be stated on some host (wms_host), which is responsible
   * for executing the workflow, and uses a scheduler (BatchStandardJobScheduler). That scheduler
   * is instantiated with the batch service, the list of hosts available for running
   * tasks, and also provided a pointer to the simulation object.
   *
   * The WMS implementation is in SimpleWMS.[cpp|h].
   */
  std::cerr << "Instantiating a WMS on " << wms_host << "..." << std::endl;
  wrench::WMS *wms = simulation.add(
          new wrench::SimpleWMS(std::unique_ptr<wrench::BatchStandardJobScheduler>(
                  new wrench::BatchStandardJobScheduler()),
                                nullptr,
                                compute_services, storage_services, wms_host));

  wms->addWorkflow(&workflow);

  /* Instantiate a file registry service to be started on some host. This service is
   * essentially a replica catalog that stores <file , storage service> pairs so that
   * any service, in particular a WMS, can discover where workflow files are stored.
   */
  std::string file_registry_service_host = hostname_list[(hostname_list.size() > 2) ? 1 : 0];
  std::cerr << "Instantiating a FileRegistryService on " << file_registry_service_host << "..." << std::endl;
  wrench::FileRegistryService *file_registry_service =
          simulation.add(new wrench::FileRegistryService(file_registry_service_host));

  /* It is necessary to store, or "stage", input files for the first task(s) of the workflow on some storage
   * service, so that workflow execution can be initiated. The getInputFiles() method of the Workflow class
   * returns the set of all workflow files that are not generated by workflow tasks, and thus are only input files.
   * These files are then staged on the storage service.
   */
  std::cerr << "Staging input files..." << std::endl;
  std::map<std::string, wrench::WorkflowFile *> input_files = workflow.getInputFiles();
  try {
    simulation.stageFiles(input_files, storage_service);
  } catch (std::runtime_error &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 0;
  }

  /* Launch the simulation. This call only returns when the simulation is complete. */
  std::cerr << "Launching the Simulation..." << std::endl;
  try {
    simulation.launch();
  } catch (std::runtime_error &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 0;
  }
  std::cerr << "Simulation done!" << std::endl;

  /* Simulation results can be examined via simulation.output, which provides access to traces
   * of events. In the code below, we retrieve the trace of all task completion events, print how
   * many such events there are, and print some information for the first such event.
   */
  std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampTaskCompletion> *> trace;
  trace = simulation.output.getTrace<wrench::SimulationTimestampTaskCompletion>();
  std::cerr << "Number of entries in TaskCompletion trace: " << trace.size() << std::endl;
  std::cerr << "Task in first trace entry: " << trace[0]->getContent()->getTask()->getId() << std::endl;

  return 0;
}
