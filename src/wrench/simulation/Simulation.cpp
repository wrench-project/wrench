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

#include "compute_services/multicore_job_executor/MulticoreJobExecutor.h"
#include "logging/TerminalOutput.h"
#include "simulation/Simulation.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simulation, "Log category for Simulation");

namespace wrench {

    /* Exception handler to catch SIGBART signals from SimGrid (which should
     * probably throw exceptions at some point)
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
     *
     */
    Simulation::Simulation() {

      // Customize the logging format
      // xbt_log_control_set("root.fmt:[%d][%h:%t(%i)]%e%m%n");
      xbt_log_control_set("root.fmt:[%d][%h:%t]%e%m%n");

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
//       this->s4u_simulation->shutdown();
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
        throw std::invalid_argument("Invalid argc argument (must be >= 1)");
      }
      if ((argv == nullptr) || (*argv == nullptr)) {
        throw std::invalid_argument("Invalid argument argv (nullptr)");
      }

      int i;
      int skip = 0;
      for (i = 1; i < *argc; i++) {
        if (!strncmp(argv[i], "--wrench-no-color", strlen("--wrench-no-color"))) {
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
      if (!this->s4u_simulation->isInitialized()) {
        throw std::runtime_error("Simulation is not initialized");
      }
      static bool already_setup = false;
      if (already_setup) {
        throw std::runtime_error("Platform already setup");
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
		 *
		 */
    void Simulation::launch() {
      // Check that the simulation is initialized
      if (!this->s4u_simulation->isInitialized()) {
        throw std::runtime_error("Simulation is not initialized");
      }

      // Check that a WMS is running
      if (!this->wms) {
        throw std::runtime_error("A WMS should have been instantiated and passed to Simulation.setWMS()");
      }

      // Check that at least one ComputeService is running
      if (this->running_compute_services.size() <= 0) {
        throw std::runtime_error(
                "At least one ComputeService should have been instantiated add passed to Simulation.add()");
      }

      // Check that at least one StorageService is running
      if (this->running_storage_services.size() <= 0) {
        throw std::runtime_error(
                "At least one StorageService should have been instantiated add passed to Simulation.add()");
      }

      // Check that a FileRegistryService is running
      if (!this->file_registry_service) {
        throw std::runtime_error(
                "A FileRegistryService should have been instantiated and passed to Simulation.setFileRegistryService()");
      }

      // Check that each input file is staged somewhere
      for (auto f : this->wms->workflow->getInputFiles()) {
        if (this->file_registry_service->entries.find(f) == this->file_registry_service->entries.end()) {
          throw std::runtime_error(
                  "Workflow input file " + f->getId() + " is not staged on any storage service!");
        }
      }

      this->s4u_simulation->runSimulation();
    }

    /**
     * @brief Adds a ComputeService to the simulation
     *
     * @param executor: a unique pointer to a ComputeService object, the ownership of which is
     *        then transferred to WRENCH
     *
     * @return a raw pointer to the ComputeService object
     *
     * @throw std::runtime_error
     */
    ComputeService *Simulation::add(std::unique_ptr<ComputeService> service) {
      if (!this->s4u_simulation->isInitialized()) {
        throw std::runtime_error("Simulation is not initialized");
      }
      ComputeService *raw_ptr = service.get();

      service->setSimulation(this);
      // Add a unique ptr to the list of Compute Services
      running_compute_services.push_back(std::move(service));
      return raw_ptr;
    }

    /**
    * @brief Adds a StorageService to the simulation
    *
    * @param executor: a unique pointer to a StorageService object, the ownership of which is
    *        then transferred to WRENCH
    *
    * @return a raw pointer to the StorageService object
     *
    * @throw std::runtime_error
    */
    StorageService *Simulation::add(std::unique_ptr<StorageService> service) {
      if (!this->s4u_simulation->isInitialized()) {
        throw std::runtime_error("Simulation is not initialized");
      }
      StorageService *raw_ptr = service.get();


      service->setSimulation(this);
      // Add a unique ptr to the list of Compute Services
      running_storage_services.push_back(std::move(service));
      return raw_ptr;
    }

    /**
     * @brief Set a WMS for the simulation
     *
     * @param wms: a unique pointer to a WMS object
     */
    void Simulation::setWMS(std::unique_ptr<WMS> wms) {
      this->wms = std::move(wms);
    }

    /**
     * @brief Set a FileRegistryService for the simulation
     *
     * @param file_registry_service: a unique pointer to a FileRegistryService object
     */
    void Simulation::setFileRegistryService(std::unique_ptr<FileRegistryService> file_registry_service) {
      this->file_registry_service = std::move(file_registry_service);
    }


    /**
     * @brief Obtain the list of compute services
     *
     * @return a vector of raw pointers to ComputeSergice objects
     */
    std::set<ComputeService *> Simulation::getComputeServices() {
      std::set<ComputeService *> set = {};
      for (auto it = this->running_compute_services.begin(); it != this->running_compute_services.end(); it++) {
        set.insert((*it).get());
      }
      return set;
    }

    /**
     * @brief Shutdown all running compute services on the platform
     */
    void Simulation::shutdownAllComputeServices() {

      for (int i = 0; i < this->running_compute_services.size(); i++) {
        this->running_compute_services[i]->stop();
      }
    }

    /**
    * @brief Obtain the list of storage services
    *
    * @return a vector of raw pointers to StorageService objects
    */
    std::set<StorageService *> Simulation::getStorageServices() {
      std::set<StorageService *> set = {};
      for (auto it = this->running_storage_services.begin(); it != this->running_storage_services.end(); it++) {
        set.insert((*it).get());
      }
      return set;
    }

    /**
     * @brief Shutdown all running storage services on the platform
     */
    void Simulation::shutdownAllStorageServices() {

      for (int i = 0; i < this->running_storage_services.size(); i++) {
        this->running_storage_services[i]->stop();
      }
    }

    /**
     * @brief Retrieves the FileRegistryService
     * @return a raw pointer to the FileRegistryService instance
     */
    FileRegistryService *Simulation::getFileRegistryService() {
      return this->file_registry_service.get();
    }


    /**
     * @brief Create an unregistered executor (i.e., that the Simulation instance will not know anything about)
     *
     * @param hostname: the hostname in the simulated platform
     * @param supports_standard_jobs: true if the executor supports StandardJob submissions, false otherwise
     * @param support_pilot_jobs: true if the executor supports PilotJob submissions, false otherwise
     * @param plist: a property (<string,string>) list
     * @param num_cores: the number of cores
     * @param ttl: the time-to-live of the executor
     * @param suffix: a suffix to be appended to the process name (useful for debugging)
     *
     * @throw std::invalid_argument
     */
    MulticoreJobExecutor *Simulation::createUnregisteredMulticoreJobExecutor(std::string hostname,
                                                                             bool supports_standard_jobs,
                                                                             bool supports_pilot_jobs,
                                                                             std::map<MulticoreJobExecutor::Property, std::string> plist,
                                                                             unsigned int num_cores,
                                                                             double ttl,
                                                                             PilotJob *pj,
                                                                             std::string suffix) {

      // Create the compute service
      MulticoreJobExecutor *executor;
      try {
        executor = new MulticoreJobExecutor(hostname, plist, num_cores, ttl, pj, suffix);
        executor->setSupportStandardJobs(supports_standard_jobs);
        executor->setSupportPilotJobs(supports_pilot_jobs);
      } catch (std::invalid_argument e) {
        throw e;
      }
      return executor;
    }


    /**
     * @brief Remove a compute service from the list of known compute services
     *
     * @param cs: a raw pointer to a ComputeService object
     */
    void Simulation::mark_compute_service_as_terminated(ComputeService *compute_service) {
      for (int i = 0; i < this->running_compute_services.size(); i++) {
        if (this->running_compute_services[i].get() == compute_service) {
          this->terminated_compute_services.push_back(std::move(this->running_compute_services[i]));
          this->running_compute_services.erase(this->running_compute_services.begin() + i);
          return;
        }
      }
      // If we didn't find the service, this means it was a hidden service that was
      // used as a building block for another higher-level service, which is fine
      return;
    }

    /**
    * @brief Remove a storage service from the list of known storage services
    *
    * @param ss: a raw pointer to a StorageService object
    */
    void Simulation::mark_storage_service_as_terminated(StorageService *storage_service) {
      for (int i = 0; i < this->running_storage_services.size(); i++) {
        if (this->running_storage_services[i].get() == storage_service) {
          this->terminated_storage_services.push_back(std::move(this->running_storage_services[i]));
          this->running_storage_services.erase(this->running_storage_services.begin() + i);
          return;
        }
      }
      // If we didn't find the service, this means it was a hidden service that was
      // used as a building block for another higher-level service, which is fine
      // (not sure this can ever happen for a StorageService, but whatever)
      return;
    }


    /**
     * @brief Stage a copy of a file on a storage service
     * @param file: a raw pointer to a WorkflowFile object
     * @param storage_service: a raw pointer to a StorageService object
     *
     * @throw std::runtime_error
     * @throw std::invalid_argument
     */
    void Simulation::stageFile(WorkflowFile *file, StorageService *storage_service) {
      if ((file == nullptr) || (storage_service == nullptr)) {
        throw std::invalid_argument("Invalid arguments");
      }

      // Check that a FileRegistryService has been set
      if (!this->file_registry_service) {
        throw std::runtime_error(
                "A FileRegistryService must be instantiated and passed to Simulation.setFileRegistryService() before files can be staged on storage services");
      }

      XBT_INFO("Staging file %s (%lf)", file->getId().c_str(), file->getSize());
      // Put the file on the storage service
      try {
        storage_service->storeFile(file);
      } catch (std::runtime_error e) {
        XBT_INFO("EXCEPTION!!");
        std::cerr << "WHAT = " << e.what() << std::endl;
        throw e;
      }

      // Update the file registry
      this->file_registry_service->addEntry(file, storage_service);

    }

    /**
   * @brief Stage a set of a file copies on a storage service
   * @param files: a set of raw pointers to WorkflowFile objects
   * @param storage_service: a raw pointer to a StorageService object
   *
   * @throw std::runtime_error
   * @throw std::invalid_argument
   */
    void Simulation::stageFiles(std::set<WorkflowFile *> files, StorageService *storage_service) {

      if (storage_service == nullptr) {
        throw std::invalid_argument("Invalid arguments");
      }

      // Check that a FileRegistryService has been set
      if (!this->file_registry_service) {
        throw std::runtime_error(
                "A FileRegistryService must be instantiated and passed to Simulation.setFileRegistryService() before files can be staged on storage services");
      }

      try {
        for (auto f : files) {
          this->stageFile(f, storage_service);
        }
      } catch (std::runtime_error e) {
        throw e;
      } catch (std::invalid_argument e) {
        throw e;
      }
    }

};