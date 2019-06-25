/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include <iomanip>
#include <csignal>
#include <simgrid/plugins/live_migration.h>

#include <wrench/wms/WMS.h>
#include "wrench/services/compute/cloud/CloudComputeService.h"
#include "wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeService.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/Service.h"
#include "wrench/services/compute/bare_metal/BareMetalComputeService.h"
#include "wrench/services/file_registry/FileRegistryService.h"
#include "wrench/services/storage/StorageService.h"
#include "wrench/simulation/Simulation.h"
#include "simgrid/plugins/energy.h"
#include "wrench/simgrid_S4U_util/S4U_VirtualMachine.h"

#ifdef MESSAGE_MANAGER
#include <wrench/util/MessageManager.h>
#endif

#include <nlohmann/json.hpp>
#include <fstream>

WRENCH_LOG_NEW_DEFAULT_CATEGORY(simulation, "Log category for Simulation");

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
        xbt_log_control_set("root.fmt:[%.20d][%h:%t(%i)]%e%m%n");

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
        // Clearing all tracked service, which will cause all services that are not
        // pointed to by main() to be deleted.
        Service::clearTrackedServices();
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


        // Extract WRENCH-specific argument
        int i;
        bool simgrid_help_requested = false;
        bool wrench_help_requested = false;
        bool simulator_help_requested = false;
        bool version_requested = false;
        bool wrench_no_log = false;

        std::vector<std::string> cleanedup_args;

        for (i = 0; i < *argc; i++) {
            if (not strcmp(argv[i], "--wrench-no-color")) {
                TerminalOutput::disableColor();
            } else if (not strcmp(argv[i], "--wrench-no-log")) {
                TerminalOutput::disableColor();
                TerminalOutput::disableLog();
                wrench_no_log = true;
            } else if (not strcmp(argv[i], "--activate-energy")) {
                sg_host_energy_plugin_init();
            } else if (not strcmp(argv[i], "--help-wrench")) {
                wrench_help_requested = true;
            } else if (not strcmp(argv[i], "--help")) {
                simulator_help_requested = true;
            } else if (not strcmp(argv[i], "--help-simgrid")) {
                simgrid_help_requested = true;
            } else if (not strcmp(argv[i], "--version")) {
                version_requested = true;
            } else {
                cleanedup_args.push_back(std::string(argv[i]));
            }
        }

        // Always activate VM migration plugin
        sg_vm_live_migration_plugin_init();

        if (wrench_help_requested) {
            std::cout << "General WRENCH command-line arguments:\n";
            std::cout << "   --wrench-no-color: disables colored terminal output\n";
            std::cout << "   --wrench-no-log: disables logging\n";
            std::cout << "     (use --help-logs for detailed help on SimGrid's logging options/syntax)\n";
            std::cout << "   --activate-energy: activates SimGrid's energy plugin\n";
            std::cout << "     (requires host pstate definitions in XML platform description file)\n";
            std::cout << "   --help-simgrid: show full help on general Simgrid command-line arguments\n";
            std::cout << "   --help-wrench: displays this help message\n";
            std::cerr << "\n";
        }

        *argc = 0;
        for (auto a : cleanedup_args) {
            argv[*argc] = strdup(a.c_str());
            (*argc)++;
        }

        // If version requested, put back the "--version" argument
        if (version_requested) {
            std::cout << "WRENCH version " << getWRENCHVersionString() << "\n";
            argv[*argc] = strdup("--version");
            (*argc)++;
        }

        // reconstruct argc/argv

        // If SimGrid help is requested, put back in a "--help" argument
        if (simgrid_help_requested) {
            argv[*argc] = strdup("--help");
            (*argc)++;
            std::cout << "\nSimgrid command-line arguments:\n\n";
        }

        // If WRENCH no logging is requested, put back and convert it to a SimGrid argument
        if (wrench_no_log) {
            argv[*argc] = strdup("--log=root.threshold:critical");
            (*argc)++;
        }

        this->s4u_simulation->initialize(argc, argv);

        if (wrench_help_requested) {
            exit(0);
        }

        // If simulator help requested, put back in the "--help" argument that was passed down
        if (simulator_help_requested) {
            argv[*argc] = strdup("--help");
            (*argc)++;
        }

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
     * @brief Get the list of names of all the hosts in the platform
     *
     * @return a vector of hostnames
     *
     */
    std::vector<std::string> Simulation::getHostnameList() {
        return S4U_Simulation::getAllHostnames();
    }

    /**
     * @brief Get the list of names of all the hosts in each cluster composing the platform
     *
     * @return a map of lists of hosts, indexed by cluster name
     *
     */
    std::map<std::string, std::vector<std::string>> Simulation::getHostnameListByCluster() {
        return S4U_Simulation::getAllHostnamesByCluster();
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

        // Before starting the simulation, obtain the initial pstate of each host.
        // By default, it will be 0 if it is not explicitly set in the platform file.
        // Even if the energy-plugin is not activated, getCurrentPstate(hostname) can
        // still be called.
        for (const auto &hostname: this->getHostnameList()) {
            this->getOutput().addTimestamp<SimulationTimestampPstateSet>(
                    new SimulationTimestampPstateSet(hostname, getCurrentPstate(hostname))
            );
        }

        // Start all services (and the WMS)
        try {
            this->startAllProcesses();
        } catch (std::runtime_error &e) {
            throw std::runtime_error("Simulation::launch(): " + std::string(e.what()));
        }

        this->is_running = true;

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
            if (wms->getWorkflow() == nullptr) {
                throw std::runtime_error(
                        "The WMS on host '" + wms->getHostname() + "' was not given a workflow to execute");
            }
        }

        for (auto &wms : this->wmses) {
            // Check that at least one StorageService is running (only needed if there are files in the workflow),
            if (not wms->workflow->getFiles().empty()) {
                bool one_storage_service_running = false;
                for (const auto &storage_service : this->storage_services) {
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

            // Check that a FileRegistryService is running if needed
            if (not wms->workflow->getInputFiles().empty()) {
                if (this->file_registry_services.empty()) {
                    throw std::runtime_error(
                            "At least one FileRegistryService should have been instantiated and passed to Simulation.add()"
                            "because there are workflow input files to be staged.");
                }
            }

            // Check that each input file is staged on the file registry services
            for (auto f : wms->workflow->getInputFiles()) {
                for (auto frs : this->file_registry_services) {
                    if (frs->entries.find(f.second) == frs->entries.end()) {
                        throw std::runtime_error(
                                "Workflow input file " + f.second->getID() + " is not staged on any storage service!");
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
                wms->start(wms, false, false);  // Not daemonized, no auto-restart
            }

            // Start the compute services
            for (const auto &compute_service : this->compute_services) {
                compute_service->start(compute_service, true, false); // Daemonized, no auto-restart
            }

            // Start the storage services
            for (const auto &storage_service : this->storage_services) {
                storage_service->start(storage_service, true, true); // Daemonized, AUTO-RESTART
            }

            // Start the scratch services
            for (const auto &compute_service : this->compute_services) {
                if (compute_service->hasScratch()) {
                    compute_service->getScratch()->simulation = this;
                    compute_service->getScratch()->start(compute_service->getScratchSharedPtr(), true,
                                                         false); // Daemonized, no auto-restart
                }
            }

            // Start the network proximity services
            for (const auto &network_proximity_service : this->network_proximity_services) {
                network_proximity_service->start(network_proximity_service, true, false); // Daemonized, no auto-restart
            }

            // Start the file registry services
            for (const auto &frs : this->file_registry_services) {
                frs->start(frs, true, false); // Daemonized, no auto-restart
            }

        } catch (std::runtime_error &e) {
            throw;
        }
    }

    /**
     * @brief Add a ComputeService to the simulation.
     *
     * @param service: a compute service
     *
     * @throw std::invalid_argument
     */
    void Simulation::addService(std::shared_ptr<ComputeService> service) {
        if (service == nullptr) {
            throw std::invalid_argument("Simulation::addService(): invalid argument (nullptr service)");
        }
        service->simulation = this;
        this->compute_services.insert(service);
    }

    /**
     * @brief Add a NetworkProximityService to the simulation.
     *
     * @param service: a network proximity service
     *
     * @throw std::invalid_argument
     */
    void Simulation::addService(std::shared_ptr<NetworkProximityService> service) {
        if (service == nullptr) {
            throw std::invalid_argument("Simulation::addService(): invalid argument (nullptr service)");
        }
        service->simulation = this;
        this->network_proximity_services.insert(service);
    }

    /**
    * @brief Add a StorageService to the simulation.
    *
    * @param service: a storage service
    *
    * @throw std::invalid_argument
    */
    void Simulation::addService(std::shared_ptr<StorageService> service) {
        if (service == nullptr) {
            throw std::invalid_argument("Simulation::addService(): invalid argument (nullptr service)");
        }
        service->simulation = this;
        this->storage_services.insert(service);
    }


    /**
   * @brief Add a WMS to the simulation.
   *
   * @param service: a WMS
   *
   * @throw std::invalid_argument
   */
    void Simulation::addService(std::shared_ptr<WMS> service) {
        if (service == nullptr) {
            throw std::invalid_argument("Simulation::addService(): invalid argument (nullptr service)");
        }
        service->simulation = this;
        this->wmses.insert(service);
    }


    /**
      * @brief Add a FileRegistryService to the simulation.
      *
      * @param service: a file registry service
      *
      * @throw std::invalid_argument
      */
    void Simulation::addService(std::shared_ptr<FileRegistryService> service) {
        if (service == nullptr) {
            throw std::invalid_argument("Simulation::addService(): invalid argument (nullptr service)");
        }
        service->simulation = this;
        this->file_registry_services.insert(service);
    }


    /**
        * @brief Stage a copy of a file on a storage service (to the "/" partition)
        *
        * @param file: a file to stage on a storage service
        * @param storage_service: the storage service
        *
        * @throw std::runtime_error
        * @throw std::invalid_argument
        */
    void Simulation::stageFile(WorkflowFile *file, std::shared_ptr<StorageService> storage_service) {
        try {
            this->stageFile(file, storage_service, "/");
        } catch (std::runtime_error &e) {
            throw;
        } catch (std::invalid_argument &e) {
            throw;
        }
    }

    /**
     * @brief Stage a copy of a file on a storage service
     *
     * @param file: a file to stage on a storage service
     * @param storage_service: the storage service
     * @param partition: the partition on which to store the files
     *
     * @throw std::runtime_error
     * @throw std::invalid_argument
     */
    void
    Simulation::stageFile(WorkflowFile *file, std::shared_ptr<StorageService> storage_service, std::string partition) {
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

        if (partition.empty()) {
            partition = "/";
        }

//        XBT_INFO("Staging file %s (%lf)", file->getID().c_str(), file->getSize());
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
   * @brief Stage file copies on a storage service (to the "/" partition)
   *
   * @param files: a map of files (indexed by file ids) to stage on a storage service
   * @param storage_service: the storage service
   *
   * @throw std::runtime_error
   * @throw std::invalid_argument
   */
    void Simulation::stageFiles(std::map<std::string, WorkflowFile *> files,
                                std::shared_ptr<StorageService> storage_service) {
        try {
            this->stageFiles(files, storage_service, "/");
        } catch (std::runtime_error &e) {
            throw e;
        } catch (std::invalid_argument &e) {
            throw e;
        }
    }

/**
  * @brief Stage file copies on a storage service
  *
  * @param files: a map of files (indexed by file ids) to stage on a storage service
  * @param storage_service: the storage service
  * @param partition: the partition on which to store the files
  *
  * @throw std::runtime_error
  * @throw std::invalid_argument
  */
    void
    Simulation::stageFiles(std::map<std::string, WorkflowFile *> files, std::shared_ptr<StorageService> storage_service,
                           std::string partition) {

        if (storage_service == nullptr) {
            throw std::invalid_argument("Simulation::stageFiles(): Invalid arguments");
        }

        // Check that at least one  FileRegistryService has been set
        if (this->file_registry_services.empty()) {
            throw std::runtime_error(
                    "Simulation::stageFiles(): A FileRegistryService must be instantiated and passed to Simulation.add() before files can be staged on storage services");
        }

        if (partition.empty()) {
            partition = "/";
        }

        try {
            for (auto const &f : files) {
                this->stageFile(f.second, storage_service, partition);
            }
        } catch (std::runtime_error &e) {
            throw;
        } catch (std::invalid_argument &e) {
            throw;
        }
    }

    /**
     * @brief Get the current simulated date
     * @return a date
     */
    double Simulation::getCurrentSimulatedDate() {
        return S4U_Simulation::getClock();
    }

    /**
     * @brief Get the memory capacity of a host given a hostname
     * @param hostname: the hostname
     * @return a memory capacity in bytes
     */
    double Simulation::getHostMemoryCapacity(std::string hostname) {
        return S4U_Simulation::getHostMemoryCapacity(hostname);
    }

    /**
    * @brief Get the number of cores of a host given a hostname
    * @param hostname: the hostname
    * @return a number of cores
    */
    unsigned long Simulation::getHostNumCores(std::string hostname) {
        return S4U_Simulation::getHostNumCores(hostname);
    }

    /**
     * @brief Get the flop rate of one core of a host given a hostname
     * @param hostname: the hostname
     * @return a flop rate (flop / sec)
     */
    double Simulation::getHostFlopRate(std::string hostname) {
        return S4U_Simulation::getHostFlopRate(hostname);
    }

    /**
     * @brief Returns whether a host is on or not
     * @param hostname: the hostname
     * @return true or false
     */
    bool Simulation::isHostOn(std::string hostname) {
        return S4U_Simulation::isHostOn(hostname);
    }

    /**
     * @brief Get the memory capacity of the host on which the calling process is running
     * @return a memory capacity in bytes
     */
    double Simulation::getMemoryCapacity() {
        return S4U_Simulation::getMemoryCapacity();
    }

    /**
     * @brief Get the number of cores of the host on which the calling process is running
     * @return a number of cores
     */
    unsigned long Simulation::getNumCores() {
        return S4U_Simulation::getNumCores();
    }

    /**
     * @brief Get the flop rate of one core of the host on which the calling process is running
     * @return a flop rate
     */
    double Simulation::getFlopRate() {
        return S4U_Simulation::getFlopRate();
    }

    /**
     * @brief Get the name of the host on which the calling process is running
     * @return a host name
     */
    std::string Simulation::getHostName() {
        return S4U_Simulation::getHostName();
    }


    /**
     * @brief Make the calling process sleep for a number of (simulated) seconds
     * @param duration: a number of seconds
     */
    void Simulation::sleep(double duration) {
        S4U_Simulation::sleep(duration);
    }

    /**
     * @brief Make the calling process compute
     * @param flops: a number of floating point operations
     */
    void Simulation::compute(double flops) {
        S4U_Simulation::compute(flops);
    }

    /**
     * @brief Get the simulation output object
     * @return simulation output object
     */
    SimulationOutput &Simulation::getOutput() {
        return this->output;
    }

    /**
     * @brief Obtains the current energy consumption of a host and will add SimulationTimestampEnergyConsumption to
     *          simulation output if can_record is set to true
     * @param hostname: the host name
     * @param record_as_time_stamp: bool signaling whether or not to record a SimulationTimestampEnergyConsumption object
     * @return current energy consumption in joules
     * @throws std::invalid_argument
     */
    double Simulation::getEnergyConsumed(const std::string &hostname, bool record_as_time_stamp) {
        if (hostname.empty()) {
            throw std::invalid_argument("Simulation::getEnergyConsumed() requires a valid hostname");
        }

        double time_now = getCurrentSimulatedDate();
        double consumption = S4U_Simulation::getEnergyConsumedByHost(hostname);

        if (record_as_time_stamp) {
            this->getOutput().addTimestamp<SimulationTimestampEnergyConsumption>(
                    new SimulationTimestampEnergyConsumption(hostname, consumption));
        }

        return consumption;
    }

    /**
    * @brief Obtains the current energy consumption of a host and will add SimulationTimestampEnergyConsumption to
    *          simulation output if can_record is set to true
    * @param hostnames: the list of hostnames
    * @param record_as_time_stamps: whether or not to record a SimulationTimestampEnergyConsumption object for each host when this method is called
    * @return current energy consumption in joules for each host, as a map indexed by hostnames
    * @throws std::invalid_argument
    */
    std::map<std::string, double>
    Simulation::getEnergyConsumed(const std::vector<std::string> &hostnames, bool record_as_time_stamps) {
        if (hostnames.empty()) {
            throw std::invalid_argument("Simulation::getEnergyConsumed() requires a valid hostname");
        }

        std::map<std::string, double> energy_consumptions;
        for (auto const &h : hostnames) {
            energy_consumptions[h] = Simulation::getEnergyConsumed(h, record_as_time_stamps);
        }
        return energy_consumptions;
    }

    /**
     * @brief Set the power state of the host
     * @param hostname: the host name
     * @param pstate: the power state index (as specified in the platform xml description file)
     */
    void Simulation::setPstate(const std::string &hostname, int pstate) {
        S4U_Simulation::setPstate(hostname, pstate);
        this->getOutput().addTimestamp<SimulationTimestampPstateSet>(
                new SimulationTimestampPstateSet(hostname, pstate));
    }

    /**
     * @brief Get the total number of power states of a host
     * @param hostname: the host name
     * @return The number of power states available for the host (as specified in the platform xml description file)
     */
    int Simulation::getNumberofPstates(const std::string &hostname) {
        return S4U_Simulation::getNumberofPstates(hostname);
    }

    /**
     * @brief Get the current power state of a host
     * @param hostname: the host name
     * @return The index of the current pstate of the host (as specified in the platform xml description file)
     */
    int Simulation::getCurrentPstate(const std::string &hostname) {
        return S4U_Simulation::getCurrentPstate(hostname);
    }

    /**
     * @brief Get the list of power states available for a host
     * @param hostname: the host name
     * @return a list of power states available for the host (as specified in the platform xml description file)
     */
    std::vector<int> Simulation::getListOfPstates(const std::string &hostname) {
        return S4U_Simulation::getListOfPstates(hostname);
    }


    /**
     * @brief Get the minimum power consumption for the host (i.e., idling) at its current pstate
     * @param hostname: the host name
     * @return The "idling" power consumption (as specified in the platform xml description file)
     */
    double Simulation::getMinPowerConsumption(const std::string &hostname) {
        return S4U_Simulation::getMinPowerConsumption(hostname);
    }

    /**
     * @brief Get the maximum power consumption for the host (i.e., 100% utilization) at its current pstate
     * @param hostname: the host name
     * @return The "100% used" power consumption (as specified in the platform xml description file)
     */
    double Simulation::getMaxPowerConsumption(const std::string &hostname) {
        return S4U_Simulation::getMaxPowerConsumption(hostname);
    }


    /**
     * @brief Starts a new compute service during WMS execution (i.e., one that was not passed to Simulation::add() before
     *        Simulation::launch() was called). The simulation takes ownership of
     *        the reference and will call the destructor.
     * @param service: An instance of a service
     * @return A pointer to the service instance
     *
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    std::shared_ptr<ComputeService> Simulation::startNewService(ComputeService *service) {

        if (service == nullptr) {
            throw std::invalid_argument("Simulation::startNewService(): invalid argument (nullptr service)");
        }

        if (not this->is_running) {
            throw std::runtime_error("Simulation::startNewService(): simulation is not running yet");
        }

        service->simulation = this;
        std::shared_ptr<ComputeService> shared_ptr = std::shared_ptr<ComputeService>(service);
        this->compute_services.insert(shared_ptr);
        shared_ptr->start(shared_ptr, true, false); // Daemonized, no auto-restart
        if (service->hasScratch()) {
            service->getScratch()->simulation = this;
            service->getScratch()->start(service->getScratchSharedPtr(), true, false); // Daemonized, no auto-restart
        }

        return shared_ptr;
    }

    /**
     * @brief Starts a new storage service during WMS execution (i.e., one that was not passed to Simulation::add() before
     *        Simulation::launch() was called). The simulation takes ownership of
     *        the reference and will call the destructor.
     * @param service: An instance of a service
     * @return A pointer to the service instance
     *
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    std::shared_ptr<StorageService> Simulation::startNewService(StorageService *service) {

        if (service == nullptr) {
            throw std::invalid_argument("Simulation::startNewService(): invalid argument (nullptr service)");
        }

        if (not this->is_running) {
            throw std::runtime_error("Simulation::startNewService(): simulation is not running yet");
        }

        service->simulation = this;
        std::shared_ptr<StorageService> shared_ptr = std::shared_ptr<StorageService>(service);
        this->storage_services.insert(shared_ptr);
        shared_ptr->start(shared_ptr, true, false); // Daemonized, no auto-restart

        return shared_ptr;
    }

    /**
     * @brief Starts a new network proximity service during WMS execution (i.e., one that was not passed to Simulation::add() before
     *        Simulation::launch() was called). The simulation takes ownership of
     *        the reference and will call the destructor.
     * @param service: An instance of a service
     * @return A pointer to the service instance
     *
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    std::shared_ptr<NetworkProximityService> Simulation::startNewService(NetworkProximityService *service) {

        if (service == nullptr) {
            throw std::invalid_argument("Simulation::startNewService(): invalid argument (nullptr service)");
        }

        if (not this->is_running) {
            throw std::runtime_error("Simulation::startNewService(): simulation is not running yet");
        }

        service->simulation = this;
        std::shared_ptr<NetworkProximityService> shared_ptr = std::shared_ptr<NetworkProximityService>(service);
        this->network_proximity_services.insert(shared_ptr);
        shared_ptr->start(shared_ptr, true, false); // Daemonized, no auto-restart

        return shared_ptr;
    }

    /**
     * @brief Starts a new file registry service during WMS execution (i.e., one that was not passed to Simulation::add() before
     *        Simulation::launch() was called). The simulation takes ownership of
     *        the reference and will call the destructor.
     * @param service: An instance of a service
     * @return A pointer to the service instance
     *
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    std::shared_ptr<FileRegistryService> Simulation::startNewService(FileRegistryService *service) {

        if (service == nullptr) {
            throw std::invalid_argument("Simulation::startNewService(): invalid argument (nullptr service)");
        }

        if (not this->is_running) {
            throw std::runtime_error("Simulation::startNewService(): simulation is not running yet");
        }

        service->simulation = this;
        std::shared_ptr<FileRegistryService> shared_ptr = std::shared_ptr<FileRegistryService>(service);
        this->file_registry_services.insert(shared_ptr);
        shared_ptr->start(shared_ptr, true, false); // Daemonized, no auto-restart

        return shared_ptr;
    }


};
