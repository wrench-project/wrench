/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include <csignal>

#include "logging/TerminalOutput.h"
#include "services/Service.h"
#include "services/compute_services/multicore_compute_service/MulticoreComputeService.h"
#include "services/file_registry_service/FileRegistryService.h"
#include "services/storage_services/StorageService.h"
#include "simulation/Simulation.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simulation, "Log category for Simulation");

namespace wrench {

    /**
     * @brief Exception handler to catch SIGABRT signals from SimGrid (which should
     * probably throw exceptions at some point)
     *
     * @param signal: the signal number
     */
    void signal_handler(int signal) {
      if (signal == SIGABRT) {
        std::cerr << "[ ABORTING ]" << std::endl;
        std::_Exit(EXIT_FAILURE);
      } else {
        std::cerr << "Unexpected signal " << signal << " received\n";
      }
    }

    /**
     * @brief  Constructor
     */
    Simulation::Simulation() {

      // Customize the logging format
      // xbt_log_control_set("root.fmt:[%d][%h:%t(%i)]%e%m%n");
      xbt_log_control_set("root.fmt:[%d][%h:%t(%i)]%e%m%n");

      // Setup the SIGABRT handler
      auto previous_handler = std::signal(SIGABRT, signal_handler);
      if (previous_handler == SIG_ERR) {
        std::cerr << "SIGABRT handler setup failed... uncaught exceptions will lead to unclean terminations\n";
      }

      // Create the S4U simulation wrapper
      this->s4u_simulation = std::unique_ptr<S4U_Simulation>(new S4U_Simulation());

    }

    /**
     * @brief Destructor
     */
    Simulation::~Simulation() {
      this->s4u_simulation->shutdown();
    }

    /**
     * @brief Initialize the simulation, which parses out WRENCH-specific and SimGrid-specific
     * command-line arguments, if any
     *
     * @param argc: main()'s argument count
     * @param argv: main()'s argument list
     *
     * @throw std::invalid_argument
     */
    void Simulation::init(int *argc, char **argv) {

      if (*argc < 1) {
        throw std::invalid_argument("Simulation::init(): Invalid argc argument (must be >= 1)");
      }
      if ((argv == nullptr) || (*argv == nullptr)) {
        throw std::invalid_argument("Simulation::init(): Invalid argument argv (nullptr)");
      }

      int i;
      int skip = 0;
      for (i = 1; i < *argc; i++) {
        if (not strncmp(argv[i], "--wrench-no-color", strlen("--wrench-no-color"))) {
          TerminalOutput::disableColor();
          skip++;
        }
        argv[i] = argv[i + skip];
      }
      *argc = i - skip;

      this->s4u_simulation->initialize(argc, argv);
    }

    /**
     * @brief Append a SimulationEvent to the event trace
     *
     * @param event
     */
    template<class T>
    void Simulation::newTimestamp(SimulationTimestamp<T> *event) {
      this->output.addTimestamp(event);
    }

    /**
     * @brief Instantiate a simulated platform
     *
     * @param filename: the path to a SimGrid XML platform description file
     *
     * @throw std::runtime_error
     */
    void Simulation::instantiatePlatform(std::string filename) {
      if (not this->s4u_simulation->isInitialized()) {
        throw std::runtime_error("Simulation::instantiatePlatform(): Simulation is not initialized");
      }
      static bool already_setup = false;
      if (already_setup) {
        throw std::runtime_error("Simulation::instantiatePlatform(): Platform already setup");
      }
      this->s4u_simulation->setupPlatform(filename);
      already_setup = true;
    }

    /**
     * @brief Retrieve the list of names of all the hosts in the platform
     *
     * @return a vector of hostnames
     *
     */
    std::vector<std::string> Simulation::getHostnameList() {
      return this->s4u_simulation->getAllHostnames();
    }

    /**
     * @brief Launch the simulation
     *
     * @throw std::runtime_error
		 */
    void Simulation::launch() {
      // Check that the simulation is initialized
      if (not this->s4u_simulation->isInitialized()) {
        throw std::runtime_error("Simulation::launch(): Simulation is not initialized");
      }

      // Check that a platform has been setup
      if (not this->s4u_simulation->isPlatformSetup()) {
        throw std::runtime_error("Simulation::launch(): Simulation platform has not been setup");
      }

      // Check that a WMS is running
      if (not this->wms) {
        throw std::runtime_error(
                "Simulation::launch(): A WMS should have been instantiated and passed to Simulation.setWMS()");
      }

      // Check that at least one ComputeService is running
      bool one_compute_service_running = false;
      for (auto it = this->compute_services.begin(); it != this->compute_services.end(); it++) {
        if ((*it)->state == Service::UP) {
          one_compute_service_running = true;
          break;
        }
      }
      if (!one_compute_service_running) {
        throw std::runtime_error(
                "Simulation::launch(): At least one ComputeService should have been instantiated add passed to Simulation.add()");
      }

      // Check that at least one StorageService is running
      bool one_storage_service_running = false;
      for (auto it = this->storage_services.begin(); it != this->storage_services.end(); it++) {
        if ((*it)->state == Service::UP) {
          one_storage_service_running = true;
        }
      }
      if (!one_storage_service_running) {
        throw std::runtime_error(
                "Simulation::launch(): At least one StorageService should have been instantiated add passed to Simulation.add()");
      }

      // Check that a FileRegistryService is running
      if (not this->file_registry_service) {
        throw std::runtime_error(
                "Simulation::launch(): A FileRegistryService should have been instantiated and passed to Simulation.setFileRegistryService()");
      }
      // Check that each input file is staged somewhere
      for (auto f : this->wms->workflow->getInputFiles()) {
        if (this->file_registry_service->entries.find(f) == this->file_registry_service->entries.end()) {
          throw std::runtime_error(
                  "Simulation::launch(): Workflow input file " + f->getId() + " is not staged on any storage service!");
        }
      }

      try {
        this->s4u_simulation->runSimulation();
      } catch (std::runtime_error &e) {
        throw;
      }
    }

    /**
     * @brief Add a ComputeService to the simulation
     *
     * @param service: a compute service
     *
     * @return the compute service
     *
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    ComputeService *Simulation::add(std::unique_ptr<ComputeService> service) {
      if (service == nullptr) {
        throw std::invalid_argument("Simulation::add(): invalid arguments");
      }
      if (not this->s4u_simulation->isInitialized()) {
        throw std::runtime_error("Simulation::add(): Simulation is not initialized");
      }
      ComputeService *raw_ptr = service.get();

      service->setSimulation(this);
      // Add a unique ptr to the list of Compute Services
      this->compute_services.insert(std::move(service));
      return raw_ptr;
    }


    /**
    * @brief Add a StorageService to the simulation
    *
    * @param service: a storage service
    * @return the storage service
     *
    * @throw std::invalid_argument
    * @throw std::runtime_error
    */
    StorageService *Simulation::add(std::unique_ptr<StorageService> service) {
      if (service == nullptr) {
        throw std::invalid_argument("Simulation::add(): invalid arguments");
      }
      if (not this->s4u_simulation->isInitialized()) {
        throw std::runtime_error("Simulation::add(): Simulation is not initialized");
      }
      StorageService *raw_ptr = service.get();

      service->setSimulation(this);
      // Add a unique ptr to the list of Compute Services
      this->storage_services.insert(std::move(service));
      return raw_ptr;
    }

    /**
     * @brief Set a WMS for the simulation
     *
     * @param wms: a WMS
     * @return the WMS
     *
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    WMS *Simulation::setWMS(std::unique_ptr<WMS> wms) {
      if (wms == nullptr) {
        throw std::invalid_argument("Simulation::setWMS(): invalid arguments");
      }
      if (not this->s4u_simulation->isInitialized()) {
        throw std::runtime_error("Simulation::setWMS(): Simulation is not initialized");
      }

      wms->setSimulation(this);
      this->wms = std::move(wms);
      return this->wms.get();
    }

    /**
     * @brief Set a FileRegistryService for the simulation
     *
     * @param file_registry_service: a file registry service
     *
     * @throw std::invalid_argument
     */
    void Simulation::setFileRegistryService(std::unique_ptr<FileRegistryService> file_registry_service) {
      if (file_registry_service == nullptr) {
        throw std::invalid_argument("Simulation::setFileRegistryService(): invalid arguments");
      }
      this->file_registry_service = std::move(file_registry_service);
    }

    /**
     * @brief Obtain the list of compute services
     *
     * @return a vector of compute services
     */
    std::set<ComputeService *> Simulation::getRunningComputeServices() {
      std::set<ComputeService *> set = {};
      for (auto it = this->compute_services.begin(); it != this->compute_services.end(); it++) {
        if ((*it)->state == Service::UP) {
          set.insert((*it).get());
        }
      }
      return set;
    }

    /**
     * @brief Shutdown all running compute services on the platform
     */
    void Simulation::shutdownAllComputeServices() {

      for (auto it = this->compute_services.begin(); it != this->compute_services.end(); it++) {
        if ((*it)->state == Service::UP) {
          (*it)->stop();
        }
      }
    }

    /**
    * @brief Obtain the list of storage services
    *
    * @return a vector of storage services
    */
    std::set<StorageService *> Simulation::getRunningStorageServices() {
      std::set<StorageService *> set = {};
      for (auto it = this->storage_services.begin(); it != this->storage_services.end(); it++) {
        if ((*it)->state == Service::UP) {
          set.insert((*it).get());
        }
      }
      return set;
    }

    /**
     * @brief Shutdown all running storage services on the platform
     */
    void Simulation::shutdownAllStorageServices() {

      for (auto it = this->storage_services.begin(); it != this->storage_services.end(); it++) {
        if ((*it)->state == Service::UP) {
          (*it)->stop();
        }
      }
    }

    /**
     * @brief Retrieves the FileRegistryService
     *
     * @return a file registry service, or nullptr
     */
    FileRegistryService *Simulation::getFileRegistryService() {
      return this->file_registry_service.get();
    }


    /**
     * @brief Stage a copy of a file on a storage service
     *
     * @param file: a file to stage on a storage service
     * @param storage_service: the storage service
     *
     * @throw std::runtime_error
     * @throw std::invalid_argument
     */
    void Simulation::stageFile(WorkflowFile *file, StorageService *storage_service) {
      if ((file == nullptr) || (storage_service == nullptr)) {
        throw std::invalid_argument("Simulation::stageFile(): Invalid arguments");
      }

      // Check that a FileRegistryService has been set
      if (not this->file_registry_service) {
        throw std::runtime_error(
                "Simulation::stageFile(): A FileRegistryService must be instantiated and passed to Simulation.setFileRegistryService() before files can be staged on storage services");
      }

      // Check that the file is not the output of anything
      if (file->isOutput()) {
        throw std::runtime_error(
                "Simulation::stageFile(): Cannot stage a file that's the output of task that hasn't executed yet");
      }

      XBT_INFO("Staging file %s (%lf)", file->getId().c_str(), file->getSize());
      // Put the file on the storage service (not via the service daemon)
      try {
        storage_service->stageFile(file);
      } catch (std::runtime_error &e) {
        throw;
      }

      // Update the file registry
      this->file_registry_service->addEntryToDatabase(file, storage_service);
    }

    /**
   * @brief Stage a set of a file copies on a storage service
     *
   * @param files: a set of files to stage on a storage service
   * @param storage_service: the storage service
   *
   * @throw std::runtime_error
   * @throw std::invalid_argument
   */
    void Simulation::stageFiles(std::set<WorkflowFile *> files, StorageService *storage_service) {

      if (storage_service == nullptr) {
        throw std::invalid_argument("Simulation::stageFiles(): Invalid arguments");
      }

      // Check that a FileRegistryService has been set
      if (not this->file_registry_service) {
        throw std::runtime_error(
                "Simulation::stageFiles(): A FileRegistryService must be instantiated and passed to Simulation.setFileRegistryService() before files can be staged on storage services");
      }

      try {
        for (auto f : files) {
          this->stageFile(f, storage_service);
        }
      } catch (std::runtime_error &e) {
        throw;
      } catch (std::invalid_argument &e) {
        throw;
      }
    }

    /**
     * @brief Get the current simulated date
     * @return the date
     */
    double Simulation::getCurrentSimulatedDate() {
      return S4U_Simulation::getClock();
    }

};
