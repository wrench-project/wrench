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
#include <simgrid/plugins/live_migration.h>

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

      // activate VM migration plugin
      sg_vm_live_migration_plugin_init();
    }

    /**
     * @brief Instantiate a simulated platform
     *
     * @param filename: the path to a SimGrid XML platform description file
     *
     * @throw std::runtime_error
     */
    void Simulation::instantiatePlatform(std::string filename) {
      static bool already_setup = false;

      if (not this->s4u_simulation->isInitialized()) {
        throw std::runtime_error("Simulation::instantiatePlatform(): Simulation is not initialized");
      }
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
     * @brief Retrieve the list of names of all the hosts in each cluster composing the platform
     *
     * @return a map of (clustername, hostnames)
     *
     */
    std::map<std::string, std::vector<std::string>> Simulation::getHostnameListByCluster() {
      return this->s4u_simulation->getAllHostnamesByCluster();
    }
    /**
     * @brief Check that a hostname exists in the platform
     * @param hostname: a host name
     * @return true or false
     */
    bool Simulation::hostExists(std::string hostname) {
      return this->s4u_simulation->hostExists(std::move(hostname));
    }

    /**
     * @brief Launch the simulation
     *
     * @throw std::runtime_error
		 */
    void Simulation::launch() {

      // Check that the simulation is correctly initialized
      try {
        this->checkSimulationSetup();
      } catch (std::runtime_error &e) {
        throw std::runtime_error("Simulation::launch(): " + std::string(e.what()));
      }


      // Start all services (and the WMS)
      try {
        this->startAllProcesses();
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
    void Simulation::checkSimulationSetup() {

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

      for (const auto &wms : this->wmses) {
        if (not this->hostExists(wms->getHostname())) {
          throw std::runtime_error("A WMS cannot be started on host '" + wms->getHostname() + "'");
        }
        if (wms->getWorkflow() == nullptr) {
          throw std::runtime_error("The WMS on host '" + wms->getHostname() + "' was not given a workflow to execute");
        }
      }

      // Check that at each ComputeService is on valid hosts
      for (const auto &compute_service : this->compute_services) {
        if (not this->hostExists(compute_service->getHostname())) {
          throw std::runtime_error(
                  "A ComputeService cannot be started on host '" + compute_service->getHostname() + "'");
        }
      }

      for (auto &wms : this->wmses) {
        // Check that at least one StorageService is running (only needed if there are files in the workflow),
        // and that each StorageService is on a valid host
        if (not wms->workflow->getFiles().empty()) {
          bool one_storage_service_running = false;
          for (const auto &storage_service : this->storage_services) {
            if (not this->hostExists(storage_service->getHostname())) {
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
          if (this->file_registry_services.empty()) {
            throw std::runtime_error(
                    "At least one FileRegistryService should have been instantiated and passed to Simulation.add()"
                            "because there are workflow input files to be staged.");
          }
          for (auto frs : this->file_registry_services) {
            if (not this->hostExists(frs->getHostname())) {
              throw std::runtime_error(
                      "A FileRegistry service cannot be started on host '" + frs->getHostname() +
                      "'");
            }
          }
        }

        // Check that each input file is staged on the file registry services
        for (auto f : wms->workflow->getInputFiles()) {
          for (auto frs : this->file_registry_services) {
            if (frs->entries.find(f.second) == frs->entries.end()) {
              throw std::runtime_error(
                      "Workflow input file " + f.second->getId() + " is not staged on any storage service!");
            }
          }
        }
      }
    }

    /**
     * @brief Start all services
     *
     * @throw std::runtime_error
     */
    void Simulation::startAllProcesses() {

      try {
        // Start the WMSes
        for (const auto &wms : this->wmses) {
          wms->start(wms, false);
        }

        // Start the compute services
        for (const auto &compute_service : this->compute_services) {
          compute_service->start(compute_service, true);
        }

        // Start the storage services
        for (const auto &storage_service : this->storage_services) {
          storage_service->start(storage_service, true);
        }

        // Start the scratch services
        for (const auto &compute_service : this->compute_services) {
          if (compute_service->hasScratch()) {
            compute_service->getScratch()->simulation = this;
            compute_service->getScratch()->start(std::shared_ptr<StorageService>(compute_service->getScratch()), true);
          }
        }

        // Start the network proximity services
        for (const auto &network_proximity_service : this->network_proximity_services) {
          network_proximity_service->start(network_proximity_service, true);
        }

        // Start the file registry services
        for (auto frs : this->file_registry_services) {
          frs->start(frs, true);
        }

      } catch (std::runtime_error &e) {
        throw;
      }
    }

    /**
     * @brief Add a ComputeService to the simulation. The simulation takes ownership of
     *        the reference and will call the destructor.
     *
     * @param service: a compute service
     *
     * @return the ComputeService
     *
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    ComputeService * Simulation::add(ComputeService *service) {
      if (service == nullptr) {
        throw std::invalid_argument("Simulation::add(): invalid argument (nullptr service)");
      }
      if (not this->s4u_simulation->isInitialized()) {
        throw std::runtime_error("Simulation::add(): Simulation is not initialized");
      }
      service->simulation = this;
      this->compute_services.insert(std::shared_ptr<ComputeService>(service));
      return service;
    }

    /**
     * @brief Add a NetworkProximityService to the simulation. The simulation takes ownership of
     *        the reference and will call the destructor.
     *
     * @param service: a network proximity service
     *
     * @return the NetworkProximityService
     *
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    NetworkProximityService * Simulation::add(NetworkProximityService *service) {
      if (service == nullptr) {
        throw std::invalid_argument("Simulation::add(): invalid argument (nullptr service)");
      }
      if (not this->s4u_simulation->isInitialized()) {
        throw std::runtime_error("Simulation::add(): Simulation is not initialized");
      }
      service->simulation = this;
      this->network_proximity_services.insert(std::shared_ptr<NetworkProximityService>(service));
      return service;
    }


    /**
    * @brief Add a StorageService to the simulation. The simulation takes ownership of
     *        the reference and will call the destructor.
    *
    * @param service: a storage service
     *
     * @return the StorageService
     *
    * @throw std::invalid_argument
    * @throw std::runtime_error
    */
    StorageService * Simulation::add(StorageService *service) {
      if (service == nullptr) {
        throw std::invalid_argument("Simulation::add(): invalid argument (nullptr service)");
      }
      if (not this->s4u_simulation->isInitialized()) {
        throw std::runtime_error("Simulation::add(): Simulation is not initialized");
      }
      service->simulation = this;
      this->storage_services.insert(std::shared_ptr<StorageService>(service));
      return service;
    }

    /**
     * @brief Add a WMS for the simulation. The simulation takes ownership of
     *        the reference and will call the destructor.
     *
     * @param wms: a WMS
     *
     * @return the WMS
     *
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    WMS * Simulation::add(WMS *wms) {
      if (wms == nullptr) {
        throw std::invalid_argument("Simulation::add(): invalid argument (nullptr wms)");
      }
      if (not this->s4u_simulation->isInitialized()) {
        throw std::runtime_error("Simulation::add(): Simulation is not initialized");
      }
      wms->simulation = this;
      this->wmses.insert(std::shared_ptr<WMS>(wms));
      return wms;
    }

    /**
     * @brief Set a FileRegistryService for the simulation. The simulation takes ownership of
     *        the reference and will call the destructor.
     *
     * @param file_registry_service: a file registry service
     *
     * @return the FileRegistryService
     *
     * @throw std::invalid_argument
     */
    FileRegistryService * Simulation::add(FileRegistryService *file_registry_service) {
      if (file_registry_service == nullptr) {
        throw std::invalid_argument("Simulation::add(): invalid arguments");
      }
      if (not this->s4u_simulation->isInitialized()) {
        throw std::runtime_error("Simulation::add(): Simulation is not initialized");
      }
      file_registry_service->simulation = this;
      this->file_registry_services.insert(std::shared_ptr<FileRegistryService>(file_registry_service));
      return file_registry_service;
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
      if (this->file_registry_services.empty()) {
        throw std::runtime_error(
                "Simulation::stageFile(): At least one FileRegistryService must be instantiated and passed to Simulation.add() before files can be staged on storage services");
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

      // Update all file registry services
      for (auto frs : this->file_registry_services) {
        frs->addEntryToDatabase(file, storage_service);
      }
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

      // Check that at least one  FileRegistryService has been set
      if (this->file_registry_services.empty()) {
        throw std::runtime_error(
                "Simulation::stageFiles(): A FileRegistryService must be instantiated and passed to Simulation.add() before files can be staged on storage services");
      }

      try {
        for (auto const &f : files) {
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

    /**
     * @brief Get the simulation output object
     * @return simulation output object
     */
    SimulationOutput &Simulation::getOutput() {
      return this->output;
    }

};
