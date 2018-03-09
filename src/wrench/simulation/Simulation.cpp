/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include <csignal>

#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/Service.h"
#include "wrench/services/compute/multihost_multicore/MultihostMulticoreComputeService.h"
#include "wrench/services/file_registry/FileRegistryService.h"
#include "wrench/services/storage/StorageService.h"
#include "wrench/simulation/Simulation.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simulation, "Log category for Simulation");

namespace wrench {

    /**
     * \cond
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
     * \endcond
     */

    /**
     * @brief  Constructor
     */
    Simulation::Simulation() {

      // Customize the logging format
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
     * @brief Check that a hostname exists in the platform
     *
     * @return true or false
     */
    bool Simulation::hostExists(std::string hostname) {
      return this->s4u_simulation->hostExists(hostname);
    }

    /**
     * @brief Launch the simulation
     *
     * @throw std::runtime_error
		 */
    void Simulation::launch() {

      // Check that the simulation is correctly initialized
      try {
        this->check_simulation_setup();
      } catch (std::runtime_error &e) {
        throw std::runtime_error("Simulation::launch(): " + std::string(e.what()));
      }

      // Start all services (and the WMS)
      try {
        this->start_all_processes();
      } catch (std::runtime_error &e) {
        throw std::runtime_error("Simulation::launch(): " + std::string(e.what()));
      }

      // Run the simulation
      try {
        this->s4u_simulation->runSimulation();
      } catch (std::runtime_error &e) {
        throw;
      }
    }

    /**
     * @brief Check that the simulation is correctly instantiated by the user
     *
     * @throw std::runtime_exception
     */
    void Simulation::check_simulation_setup() {

      // Check that the simulation is initialized
      if (not this->s4u_simulation->isInitialized()) {
        throw std::runtime_error("Simulation is not initialized");
      }

      // Check that a platform has been setup
      if (not this->s4u_simulation->isPlatformSetup()) {
        throw std::runtime_error("Simulation platform has not been setup");
      }

      // Check that there is a WMS
      if (this->wmses.empty()) {
        throw std::runtime_error(
                "A WMS should have been instantiated and passed to Simulation.setWMS()");
      }


      for (auto it = this->wmses.begin(); it != this->wmses.end(); ++it) {
        auto wms = it->get();
        if (not this->hostExists(wms->getHostname())) {
          throw std::runtime_error("A WMS cannot be started on host '" + wms->getHostname() + "'");
        }
        if (wms->getWorkflow() == nullptr) {
          throw std::runtime_error("The WMS on host '" + wms->getHostname() + "' was not given a workflow to execute");
        }
      }

      // Check that at least one ComputeService is created, and that all services are on valid hosts
      bool one_compute_service_running = false;
      for (const auto &compute_service : this->compute_services) {
        if (compute_service->state == Service::UP) {
          one_compute_service_running = true;
          break;
        }
        if (not this->hostExists(compute_service->getHostname())) {
          throw std::runtime_error(
                  "A ComputeService cannot be started on host '" + compute_service->getHostname() + "'");
        }
      }
      if (!one_compute_service_running) {
        throw std::runtime_error(
                "At least one ComputeService should have been instantiated add passed to Simulation.add()");
      }

      for (auto it = this->wmses.begin(); it != this->wmses.end(); ++it) {
        auto wms = it->get();

        // Check that at least one StorageService is running (only needed if there are files in the workflow),
        // and that each StorageService is on a valid host
        if (not wms->workflow->getFiles().empty()) {
          bool one_storage_service_running = false;
          for (const auto &storage_service : this->storage_services) {
            if (not this->hostExists(storage_service->getHostname())) {
              WRENCH_INFO("HOST DOES NOT EXIST: %s", storage_service->getHostname().c_str());
              throw std::runtime_error(
                      "A StorageService cannot be started on host '" + storage_service->getHostname() + "'");
            }
            if (storage_service->state == Service::UP) {
              one_storage_service_running = true;
              break;
            }
          }
          if (not one_storage_service_running) {
            throw std::runtime_error(
                    "At least one StorageService should have been instantiated add passed to Simulation.add()");
          }
        }

        // Check that a FileRegistryService is running if needed, and is on a valid host
        if (not wms->workflow->getInputFiles().empty()) {
          if (not this->file_registry_service) {
            throw std::runtime_error(
                    "A FileRegistryService should have been instantiated and passed to Simulation.setFileRegistryService()"
                            "because there are workflow input files to be staged.");
          }
          if (not this->hostExists(file_registry_service->getHostname())) {
            throw std::runtime_error(
                    "A FileRegistry service cannot be started on host '" + file_registry_service->getHostname() + "'");
          }
        }

        // Check that each input file is staged somewhere
        for (auto f : wms->workflow->getInputFiles()) {
          if (this->file_registry_service->entries.find(f.second) == this->file_registry_service->entries.end()) {
            throw std::runtime_error(
                    "Workflow input file " + f.second->getId() + " is not staged on any storage service!");
          }
        }
      }
    }

    /**
     * @brief Start all services
     *
     * @throw std::runtime_error
     */
    void Simulation::start_all_processes() {

      try {
        // Start the WMSes
        for (const auto &wms : this->wmses) {
          wms->createLifeSaver(wms);
          wms->start();
        }

        // Start the compute services
        for (const auto &compute_service : this->compute_services) {
          compute_service->createLifeSaver(compute_service);
          compute_service->start(true);
        }

        // Start the storage services
        for (const auto &storage_service : this->storage_services) {
          storage_service->createLifeSaver(storage_service);
          storage_service->start(true);
        }

        // Start the network proximity services
        for (const auto &network_proximity_service : this->network_proximity_services) {
          network_proximity_service->createLifeSaver(network_proximity_service);
          network_proximity_service->start(true);
        }

        // Start the file registry service
        if (this->file_registry_service) {
          this->file_registry_service->createLifeSaver(this->file_registry_service);
          this->file_registry_service->start(true);
        }

      } catch (std::runtime_error &e) {
        throw;
      }
    }

    /**
     * @brief Add a ComputeService to the simulation
     *
     * @param service: a compute service
     *
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    ComputeService * Simulation::add(ComputeService *service) {
      if (service == nullptr) {
        throw std::invalid_argument("Simulation::add(): invalid arguments");
      }
      if (not this->s4u_simulation->isInitialized()) {
        throw std::runtime_error("Simulation::add(): Simulation is not initialized");
      }
      service->setSimulation(this);
      this->compute_services.insert(std::shared_ptr<ComputeService>(service));
      return service;
    }

    /**
     * @brief Add a NetworkProximityService to the simulation
     *
     * @param service: a network proximity service
     *
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    NetworkProximityService * Simulation::add(NetworkProximityService *service) {
      if (service == nullptr) {
        throw std::invalid_argument("Simulation::add(): invalid arguments");
      }
      if (not this->s4u_simulation->isInitialized()) {
        throw std::runtime_error("Simulation::add(): Simulation is not initialized");
      }
      service->setSimulation(this);
      this->network_proximity_services.insert(std::shared_ptr<NetworkProximityService>(service));
      return service;
    }


    /**
    * @brief Add a StorageService to the simulation
    *
    * @param service: a storage service
     *
    * @throw std::invalid_argument
    * @throw std::runtime_error
    */
    StorageService * Simulation::add(StorageService *service) {
      if (service == nullptr) {
        throw std::invalid_argument("Simulation::add(): invalid arguments");
      }
      if (not this->s4u_simulation->isInitialized()) {
        throw std::runtime_error("Simulation::add(): Simulation is not initialized");
      }
      service->setSimulation(this);
      this->storage_services.insert(std::shared_ptr<StorageService>(service));
      return service;
    }

    /**
     * @brief Add a WMS for the simulation
     *
     * @param wms: a WMS
     *
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    WMS * Simulation::add(WMS *wms) {
      if (wms == nullptr) {
        throw std::invalid_argument("Simulation::addWMS(): invalid arguments");
      }
      if (not this->s4u_simulation->isInitialized()) {
        throw std::runtime_error("Simulation::setWMS(): Simulation is not initialized");
      }
      wms->setSimulation(this);
      this->wmses.insert(std::shared_ptr<WMS>(wms));
      return wms;
    }

    /**
     * @brief Set a FileRegistryService for the simulation
     *
     * @param file_registry_service: a file registry service
     *
     * @throw std::invalid_argument
     */
    FileRegistryService * Simulation::setFileRegistryService(FileRegistryService *file_registry_service) {
      if (file_registry_service == nullptr) {
        throw std::invalid_argument("Simulation::setFileRegistryService(): invalid arguments");
      }
      this->file_registry_service = std::shared_ptr<FileRegistryService>(file_registry_service);
      return file_registry_service;
    }

    /**
     * @brief Retrieves all running network proximity services on the platform
     *
     * @return a vector of network proximity services
     */
    std::set<NetworkProximityService *> Simulation::getRunningNetworkProximityServices() {
      std::set<NetworkProximityService *> set = {};
      for (auto it = this->network_proximity_services.begin(); it != this->network_proximity_services.end(); it++) {
        if ((*it)->state == Service::UP) {
          set.insert((*it).get());
        }
      }
      return set;
    }

    /**
    * @brief Shutdown all running network proximity services on the platform
    */
    void Simulation::shutdownAllNetworkProximityServices() {

      std::cerr << "Shutting fown all network proximity services\n";
      for (auto it = this->network_proximity_services.begin(); it != this->network_proximity_services.end(); it++) {
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
   * @param files: a map of files (indexed by file ids) to stage on a storage service
   * @param storage_service: the storage service
   *
   * @throw std::runtime_error
   * @throw std::invalid_argument
   */
    void Simulation::stageFiles(std::map<std::string, WorkflowFile *> files, StorageService *storage_service) {

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
          this->stageFile(f.second, storage_service);
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

    /**
     * @brief Get the memory capacity of a host given a hostname
     * @param hostname: the hostname
     * @return the memory capacity in bytes
     */
    double Simulation::getHostMemoryCapacity(std::string hostname) {
      return S4U_Simulation::getHostMemoryCapacity(hostname);
    }

    /**
    * @brief Get the number of cores of a host given a hostname
    * @param hostname: the hostname
    * @return the number of cores
    */
    unsigned long Simulation::getHostNumCores(std::string hostname) {
      return S4U_Simulation::getNumCores(hostname);
    }

    /**
     * @brief Get the flop rate of one core of a host given a hostname
     * @param hostname: the hostname
     * @return the flop rate (flop / sec)
     */
    double Simulation::getHostFlopRate(std::string hostname) {
      return S4U_Simulation::getFlopRate(hostname);
    }

    /**
     * @brief Get the memory capacity of the current host
     * @return the memory capacity in bytes
     */
    double Simulation::getMemoryCapacity() {
      return S4U_Simulation::getMemoryCapacity();
    }

    /**
     * @brief Sleep for a number of (simulated) seconds
     * @param duration in seconds
     */
    void Simulation::sleep(double duration) {
      S4U_Simulation::sleep(duration);
    }

};
