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
#include "scheduler/CloudScheduler.h"

/**
 * @brief An example of a simple WMS within a cloud service.
 *
 * @param argc: argument count
 * @param argv: argument vector
 * @return whether the simulation has successfully completed
 */
int main(int argc, char **argv) {

  wrench::Simulation simulation;

  simulation.init(&argc, argv);

  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <xml platform file> <workflow file>" << std::endl;
    exit(1);
  }

  char *platform_file = argv[1];
  char *workflow_file = argv[2];

  // reading and parsing the workflow file
  std::cerr << "Loading workflow..." << std::endl;
  wrench::Workflow workflow;
  wrench::WorkflowUtil::loadFromDAX(workflow_file, &workflow);

  std::cerr << "The workflow has " << workflow.getNumberOfTasks() << " tasks " << std::endl;
  std::cerr.flush();

  // reading and parsing the SimGrid virtual platform description file
  std::cerr << "Instantiating SimGrid platform..." << std::endl;
  simulation.instantiatePlatform(platform_file);

  // obtaining the list of hosts
  std::vector<std::string> hostname_list = simulation.getHostnameList();

  // storage service
  std::string storage_host = hostname_list[(hostname_list.size() > 2) ? 2 : 1];
  std::cerr << "Instantiating a SimpleStorageService on " << storage_host << "..." << std::endl;
  wrench::StorageService *storage_service = simulation.add(std::unique_ptr<wrench::SimpleStorageService>(
          new wrench::SimpleStorageService(storage_host, 10000000000000.0)));

  // WMS and Cloud services in the same host
  std::string wms_host = hostname_list[0];
  wrench::ComputeService *cloud_service = new wrench::CloudService(
          wms_host, true, true, storage_service,
          {{wrench::CloudServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD, "666"}});

  try {
    simulation.add(std::unique_ptr<wrench::ComputeService>(cloud_service));

  } catch (std::invalid_argument &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    std::exit(1);
  }

  // execution hosts
  std::string executor_host = hostname_list[(hostname_list.size() > 1) ? 1 : 0];
  std::vector<std::string> execution_hosts = {executor_host};

  std::cerr << "Instantiating a WMS on " << wms_host << "..." << std::endl;

  // WMS Configuration
  wrench::WMS *wms = simulation.setWMS(
          std::unique_ptr<wrench::WMS>(
                  new wrench::SimpleWMS(&workflow,
                                        std::unique_ptr<wrench::Scheduler>(
                                                new wrench::CloudScheduler(cloud_service,
                                                                           execution_hosts,
                                                                           &simulation)),
                                        wms_host)));

  // file registry
  std::string file_registry_service_host = hostname_list[(hostname_list.size() > 2) ? 1 : 0];
  std::cerr << "Instantiating a FileRegistryService on " << file_registry_service_host << "..." << std::endl;
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(file_registry_service_host));
  simulation.setFileRegistryService(std::move(file_registry_service));

  // staging input files
  std::cerr << "Staging input files..." << std::endl;
  std::set<wrench::WorkflowFile *> input_files = workflow.getInputFiles();
  try {
    simulation.stageFiles(input_files, storage_service);
  } catch (std::runtime_error &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 0;
  }

  // launching the simulation
  std::cerr << "Launching the Simulation..." << std::endl;
  try {
    simulation.launch();
  } catch (std::runtime_error &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 0;
  }
  std::cerr << "Simulation done!" << std::endl;

  // metrics
  std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampTaskCompletion> *> trace;
  trace = simulation.output.getTrace<wrench::SimulationTimestampTaskCompletion>();
  std::cerr << "Number of entries in TaskCompletion trace: " << trace.size() << std::endl;
  std::cerr << "Task in first trace entry: " << trace[0]->getContent()->getTask()->getId() << std::endl;

  return 0;
}
