/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include <cstdlib>
#include <iomanip>
#include <csignal>
#include <simgrid/plugins/live_migration.h>

#include <wrench/execution_controller/ExecutionController.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/file_registry/FileRegistryService.h>
#include <wrench/services/storage/StorageService.h>
#include <wrench/services/storage/compound/CompoundStorageService.h>
#include <wrench/simulation/Simulation.h>
#include "simgrid/plugins/energy.h"
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>
//#include <wrench/services/memory/MemoryManager.h>
#include <wrench/data_file/DataFile.h>
#include <wrench/util/UnitParser.h>


#ifdef MESSAGE_MANAGER
#include <wrench/util/MessageManager.h>
#endif

#include <memory>
#include <utility>
#include <fstream>

WRENCH_LOG_CATEGORY(wrench_core_simulation, "Log category for Simulation");

namespace wrench {


    bool Simulation::energy_enabled = false;
    bool Simulation::host_shutdown_enabled = false;
    bool Simulation::link_shutdown_enabled = false;
    bool Simulation::pagecache_enabled = false;
    bool Simulation::surf_precision_set_by_user = false;

    bool Simulation::initialized = false;

    std::map<std::string, std::shared_ptr<DataFile>> Simulation::data_files;


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

        // Set up the SIGABRT handler
        auto previous_handler = std::signal(SIGABRT, signal_handler);
        if (previous_handler == SIG_ERR) {
            std::cerr << "SIGABRT handler setup failed... uncaught exceptions will lead to unclean terminations\n";
        }

        // Create the S4U simulation wrapper
        this->s4u_simulation = std::make_unique<S4U_Simulation>();
    }

    /**
     * @brief Destructor
     */
    Simulation::~Simulation() {
        //        this->removeAllFiles();
        this->s4u_simulation->shutdown();
    }

    /**
     * @brief Initialize the simulation, which parses out WRENCH-specific and SimGrid-specific
     * command-line arguments, if any
     *
     * @param argc: main()'s argument count
     * @param argv: main()'s argument list
     *
     */
    void Simulation::init(int *argc, char **argv) {
        if (Simulation::initialized) {
            throw std::runtime_error("Simulation::init(): Simulation already initialized");
        }

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
        bool default_control_message_size_set = false;

        // By  default, logs are disabled
        xbt_log_control_set("root.thresh:critical");

        // Determine if specific --cfg arguments were passed
        for (i = 0; i < *argc; i++) {
            if (!strncmp(argv[i], "--cfg=surf/precision:", strlen("--cfg=surf/precision:"))) {
                Simulation::surf_precision_set_by_user = true;
            }
        }

        std::vector<std::string> cleaned_up_args;
        for (i = 0; i < *argc; i++) {
            if (not strcmp(argv[i], "--help")) {
                simulator_help_requested = true;
            } else if ((not strcmp(argv[i], "--wrench-no-color")) or (not strcmp(argv[i], "--wrench-no-colors"))) {
                TerminalOutput::disableColor();
            } else if ((not strcmp(argv[i], "--wrench-full-log")) or
                       (not strcmp(argv[i], "--wrench-full-logs")) or
                       (not strcmp(argv[i], "--wrench-log-full")) or
                       (not strcmp(argv[i], "--wrench-logs-full"))) {
                xbt_log_control_set("root.thresh:info");
            } else if (not strncmp(argv[i], "--wrench-commport-pool-size", strlen("--wrench-commport-pool-size"))) {
                char *equal_sign = strchr(argv[i], '=');
                if (!equal_sign) {
                    throw std::invalid_argument("Invalid --wrench-commport-pool-size argument value");
                }
                // Check that the value is all digits
                char *ptr = equal_sign + 1;
                while (*ptr) {
                    if (*ptr < '0' or *ptr > '9') {
                        throw std::invalid_argument("Invalid --wrench-commport-pool-size argument value");
                    }
                    ptr++;
                }
                unsigned long pool_size = strtoul(equal_sign + 1, nullptr, 10);
                S4U_CommPort::commport_pool_size = pool_size;
            } else if (not strncmp(argv[i], "--wrench-default-control-message-size", strlen("--wrench-default-control-message-size"))) {
                char *equal_sign = strchr(argv[i], '=');
                if (!equal_sign) {
                    throw std::invalid_argument("Invalid --wrench-default-control-message-size argument value");
                }
                // Check that the value is all digits
                char *ptr = equal_sign + 1;
                while (*ptr) {
                    if (*ptr < '0' or *ptr > '9') {
                        throw std::invalid_argument("Invalid --wrench-default-control-message-size argument value");
                    }
                    ptr++;
                }
                S4U_CommPort::default_control_message_size = strtoll(equal_sign + 1, nullptr, 10);
                default_control_message_size_set = true;
            } else if (not strcmp(argv[i], "--wrench-energy-simulation")) {
                sg_host_energy_plugin_init();
                Simulation::energy_enabled = true;
            } else if (not strcmp(argv[i], "--wrench-host-shutdown-simulation")) {
                Simulation::host_shutdown_enabled = true;
            } else if (not strcmp(argv[i], "--wrench-link-shutdown-simulation")) {
                Simulation::link_shutdown_enabled = true;
            } else if ((not strcmp(argv[i], "--help-wrench")) or
                       (not strcmp(argv[i], "--wrench-help"))) {
                wrench_help_requested = true;
            } else if (not strcmp(argv[i], "--help-simgrid")) {
                simgrid_help_requested = true;
            } else if (not strcmp(argv[i], "--wrench-version")) {
                version_requested = true;
            } else if (not strcmp(argv[i], "--wrench-pagecache-simulation")) {
                throw std::invalid_argument("The use of --wrench-pagecache-simulation has been deprecated for now");
                Simulation::pagecache_enabled = true;
            } else {
                cleaned_up_args.emplace_back(argv[i]);
            }
        }

        // Always activate VM migration plugin
        sg_vm_live_migration_plugin_init();

        // Register a callback on host state changes to warn users
        // that the --wrench-host-shutdown-simulation flag should have been passed
        // to the simulator if host shutdowns are to be simulated
        simgrid::s4u::Host::on_onoff.connect(
                [](simgrid::s4u::Host const &h) {
                    if (not Simulation::host_shutdown_enabled) {
                        throw std::runtime_error(
                                "It looks like you are simulating host failures/shutdowns during the simulated execution."
                                " Please restart your simulation passing it the --wrench-host-shutdown-simulation command-line flag.");
                    }
                });

        // Register a callback on link state changes to warn users
        // that the --wrench-host-shutdown-simulation flag should have been passed
        // to the simulator if host shutdowns are to be simulated
        simgrid::s4u::Link::on_onoff_cb(
                [](simgrid::s4u::Link const &l) {
                    if (not Simulation::link_shutdown_enabled) {
                        throw std::runtime_error(
                                "It looks like you are simulating link failures/shutdowns during the simulated execution."
                                " Please restart your simulation passing it the --wrench-link-shutdown-simulation command-line flag.");
                    }
                });


        // Print help message if requested
        if (wrench_help_requested) {
            std::cout << "General WRENCH command-line arguments:\n";
            std::cout << "   --wrench-help: displays this help message\n";
            std::cout << "   --wrench-energy-simulation: activates SimGrid's energy plugin\n";
            std::cout << "     (requires host pstate definitions in XML platform description file)\n";
            std::cout
                    << "   --wrench-host-shutdown-simulation: activates WRENCH's capability to simulate host failures/shutdowns during execution\n";
            std::cout
                    << "   --wrench-link-shutdown-simulation: activates WRENCH's capability to simulate link failures/shutdowns during execution\n";
            std::cout
                    << "   --wrench-pagecache-simulation: Activate the in-memory (Linux) page cache simulation (which ";
            std::cout
                    << "                requires that all hosts in the platform have a disk mounted at '/memory' that )\n";
            std::cout << "                acts as additional RAM that can only be used for caching file pages)\n";
            std::cout << "   --wrench-no-color: disables colored terminal output\n";
            std::cout << "   --wrench-full-log: enables full logging\n";
            std::cout << "     (use --log=xxx.threshold=info to enable log category xxxx)\n";
            std::cout << "   --help-logs for detailed help on (SimGrid's) logging options/syntax)\n";
            std::cout << "   --help-simgrid: show full help on general Simgrid command-line arguments\n";
            std::cout << "   --wrench-commport-pool-size=<integer>: set the number of communication ports used by WRENCH (default: 50000).\n";
            std::cout << "      This value may need to be increased, especially for simulations that simulate many\n";
            std::cout << "      failures, for which WRENCH has a hard time avoiding all commport-related memory leaks\n";
            std::cout << "   --wrench-default-control-message-size=<int>: the default size of control messages in bytes (default: 0). \n";
            std::cerr << "\n";
        }


        *argc = 0;
        for (const auto &a: cleaned_up_args) {
            //free(argv[(*argc)]);//you cant free the base args, so no one is going to try to free ours.
            //This is just going to have to stay a memory leak
            argv[(*argc)] = strdup(a.c_str());
            (*argc)++;
        }

        // If version requested, put back the "--version" argument
        if (version_requested) {
            std::cout << "This program was linked against WRENCH version " << Simulation::getWRENCHVersionString();
#ifdef ENABLE_BATSCHED
            std::cout << " (compiled with Batsched)";
#endif
            std::cout << "\n\nTo cite WRENCH in a publication, please use:\n"
                         "  @article{wrench,\n"
                         "    title = {{Developing Accurate and Scalable Simulators of Production Workflow Management Systems with WRENCH}},\n"
                         "    author = {Casanova, Henri and Ferreira da Silva, Rafael and Tanaka, Ryan and Pandey, Suraj and Jethwani, Gautam and Koch, William and Albrecht, Spencer and Oeth, James and Suter, Frederic},\n"
                         "    journal = {Future Generation Computer Systems},\n"
                         "    volume = {112},\n"
                         "    number = {},\n"
                         "    year = {2020},\n"
                         "    pages = {162--175},\n"
                         "    doi = {10.1016/j.future.2020.05.030}\n"
                         "  }\n\n"
                         "-----------------------------------------------------------------------------------\n\n";
            // put the argument back in so that we also see the
            // SimGrid message
            argv[(*argc)] = strdup("--version");
            (*argc)++;
        }

        // reconstruct argc/argv

        // If SimGrid help is requested, put back in a "--help" argument
        if (simgrid_help_requested) {
            argv[(*argc)] = strdup("--help");
            (*argc)++;
            std::cout << "\nSimgrid command-line arguments:\n\n";
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

        if (not default_control_message_size_set) {
            S4U_CommPort::default_control_message_size = 0;
        }

        if (Simulation::link_shutdown_enabled and S4U_CommPort::default_control_message_size == 0) {
            throw std::invalid_argument("The use of --wrench-link-shutdown-simulation requires that the default size of control messages be non-zero. "
                                        "Use  --wrench-default-control-message-size=xxx to set the default control message size. ");
        }

        Simulation::initialized = true;
    }

    /**
     * @brief Instantiate a simulated platform
     *
     * @param filename: the path to a SimGrid XML platform description file (.xml) or shared object platform file (.so/.dylib)
     *
     */
    void Simulation::instantiatePlatform(const std::string &filename) {
        if (not this->s4u_simulation->isInitialized()) {
            throw std::runtime_error("Simulation::instantiatePlatform(): Simulation is not initialized");
        }
        if (already_setup) {
            throw std::runtime_error("Simulation::instantiatePlatform(): Platform already setup");
        }

        this->s4u_simulation->setupPlatformFromFile(filename);

        this->platformSanityCheck();

        this->already_setup = true;
    }

    /**
     * @brief Instantiate a simulated platform
     *
     * @param platform: the string representation of a SimGrid XML platform description
     *
     */
    void Simulation::instantiatePlatformFromString(const std::string &platform) {
        auto const temp_dir = std::filesystem::temp_directory_path();
        auto const temp_path = temp_dir / ("tmp_wrench_platform_" + to_string(::getpid()) + "_" + to_string(std::time(nullptr)) + ".xml");
        ofstream file;
        file.open(temp_path);
        file << platform << std::endl;
        file.close();

        instantiatePlatform(temp_path);
        remove(temp_path);
    }

    /**
     * @brief Instantiate a simulated platform
     *
     * @param creation_function void() function to create the platform
     */
    void Simulation::instantiatePlatform(const std::function<void()> &creation_function) {
        if (not this->s4u_simulation->isInitialized()) {
            throw std::runtime_error("Simulation::instantiatePlatform(): Simulation is not initialized");
        }
        if (already_setup) {
            throw std::runtime_error("Simulation::instantiatePlatform(): Platform already setup");
        }

        this->s4u_simulation->setupPlatformFromLambda(creation_function);
        this->platformSanityCheck();
        this->already_setup = true;
    }


    /**
     * @brief Get the list of names of all the physical hosts in the platform
     *
     * @return a vector of hostnames
     */
    std::vector<std::string> Simulation::getHostnameList() {
        return S4U_Simulation::getAllHostnames();
    }

    /**
     * @brief Get the list of link names on the route between two hosts
     * @param src_host: src hostname
     * @param dst_host: dst hostname
     * @return a vector of link names
     */
    std::vector<std::string> Simulation::getRoute(std::string &src_host, std::string &dst_host) {
        return S4U_Simulation::getRoute(src_host, dst_host);
    }

    /**
     * @brief Get the list of names of all the links in the platform
     *
     * @return a vector of link names
     */
    std::vector<std::string> Simulation::getLinknameList() {
        return S4U_Simulation::getAllLinknames();
    }

    /**
     * @brief Get the max bandwidth of a particular link
     * @param link_name: the link's name
     *
     * @return a bandwidth in Bps
     */
    double Simulation::getLinkBandwidth(const std::string &link_name) {
        return S4U_Simulation::getLinkBandwidth(link_name);
    }

    /**
     * @brief Get the current usage of a particular link
     * @param link_name: the link's name
     *
     * @return a bandwidth usage
     */
    double Simulation::getLinkUsage(const std::string &link_name) {
        return S4U_Simulation::getLinkUsage(link_name);
    }

    /**
     * @brief Obtains the current link bandwidth usage on a link and will add SimulationTimestampLinkUsage to
     *        simulation output if record_as_time_stamp is set to true
     *        
     * @param link_name: the link's name
     * @param record_as_time_stamp: bool signaling whether or not to record a SimulationTimestampLinkUsage object
     * @return current bandwidth usage in Bps
     */
    double Simulation::getLinkUsage(const std::string &link_name, bool record_as_time_stamp) {
        if (link_name.empty()) {
            throw std::invalid_argument("Simulation::getLinkUsage() requires a valid link name");
        }

        const double usage = S4U_Simulation::getLinkUsage(link_name);
        if (record_as_time_stamp) {
            this->getOutput().addTimestampLinkUsage(Simulation::getCurrentSimulatedDate(), link_name, usage);
        }
        return usage;
    }

    /**
     * @brief Method to check if page caching is activated
     * @return true or false
     */
    bool Simulation::isPageCachingEnabled() {
        return Simulation::pagecache_enabled;
    }

    /**
     * @brief Method to check if host shutdown simulation is activated
     * @return true or false
     */
    bool Simulation::isHostShutdownSimulationEnabled() {
        return Simulation::host_shutdown_enabled;
    }

    /**
     * @brief Method to check if link shutdown simulation is activated
     * @return true or false
     */
    bool Simulation::isLinkShutdownSimulationEnabled() {
        return Simulation::link_shutdown_enabled;
    }


    /**
     * @brief Method to check if energy simulation is activated
     * @return true or false
     */
    bool Simulation::isEnergySimulationEnabled() {
        return Simulation::energy_enabled;
    }

    /**
     * @brief Method to check if surf precision was set by the user
     * @return true or false
     */
    bool Simulation::isSurfPrecisionSetByUser() {
        return Simulation::surf_precision_set_by_user;
    }


    ///**
    // * @brief Method to retrieve MemoryManager by hostname
    // * @param hostname
    // * @return
    // */
    ////    MemoryManager* Simulation::getMemoryManagerByHost(std::string hostname) {
    ////        for (auto mem_mng: this->memory_managers) {
    ////            if (strcmp(mem_mng.get()->getHostname().c_str(), hostname.c_str()) == 0) {
    ////                return mem_mng.get();
    ////            }
    ////        }
    ////        return nullptr;
    ////    }

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
            this->getOutput().addTimestampPstateSet(Simulation::getCurrentSimulatedDate(), hostname, getCurrentPstate(hostname));
        }

        // Start all services (including execution controllers)
        try {
            this->startAllProcesses();
        } catch (std::runtime_error &e) {
            throw std::runtime_error("Simulation::launch(): " + std::string(e.what()));
        }

        // Run the simulation
        try {
            this->is_running = true;
            this->s4u_simulation->runSimulation();
            wrench::FileLocation::file_location_map.clear();
            //            Service::deleteLifeSaversOfAutorestartServices();
            this->is_running = false;
        } catch (std::runtime_error &) {
            wrench::FileLocation::file_location_map.clear();
            //            Service::deleteLifeSaversOfAutorestartServices();
            this->is_running = false;
            throw;
        }
    }

        /**
         * @brief Checks whether the simulation is running or not
         *
         * @return true or false
         */
        bool Simulation::isRunning() const {
            return this->is_running;
        }

    /**
     * @brief Check that the simulation is correctly instantiated by the user
     *
     */
    void Simulation::checkSimulationSetup() {
        // Check that the simulation is initialized
        if (not this->s4u_simulation->isInitialized()) {
            throw std::runtime_error("Simulation is not initialized");
        }

        // Check that a platform has been set up
        if (not this->s4u_simulation->isPlatformSetup()) {
            throw std::runtime_error("Simulation platform has not been setup");
        }

        // Check that there is at least one execution controller
        if (this->execution_controllers.empty()) {
            throw std::runtime_error(
                    "An execution controller should have been instantiated and passed to Simulation.add()");
        }
    }

    /**
     * @brief Start all services
     *
     */
    void Simulation::startAllProcesses() {

        // Start the execution controllers
        for (const auto &execution_controller: this->execution_controllers) {
            execution_controller->start(execution_controller, execution_controller->daemonized_, false);// Not daemonized, no auto-restart
        }

        // Start the compute services
        for (const auto &compute_service: this->compute_services) {
            compute_service->start(compute_service, true, false);// Daemonized, no auto-restart
        }

        // Start the storage services
        for (const auto &storage_service: this->storage_services) {
            storage_service->start(storage_service, true, true);// Daemonized, AUTO-RESTART
        }


        //            // Start the scratch services
        //            for (const auto &compute_service: this->compute_services) {
        //                if (compute_service->hasScratch()) {
        //                    compute_service->getScratch()->simulation = this;
        //                    compute_service->getScratch()->start(compute_service->getScratchSharedPtr(), true,
        //                                                         false);// Daemonized, no auto-restart
        //                }
        //            }

        // Start the network proximity services
        for (const auto &network_proximity_service: this->network_proximity_services) {
            network_proximity_service->start(network_proximity_service, true, false);// Daemonized, no auto-restart
        }

        // Start the file registry services
        for (const auto &frs: this->file_registry_services) {
            frs->start(frs, true, false);// Daemonized, no auto-restart
        }

        // Start the energy meter services
        for (const auto &frs: this->energy_meter_services) {
            frs->start(frs, true, false);// Daemonized, no auto-restart
        }
    }

    /**
     * @brief Add a ComputeService to the simulation.
     *
     * @param service: a compute service
     *
     */
    void Simulation::addService(const std::shared_ptr<ComputeService> &service) {
        if (service == nullptr) {
            throw std::invalid_argument("Simulation::addService(): invalid argument (nullptr service)");
        }
        service->simulation_ = this;
        this->compute_services.insert(service);
    }

    /**
     * @brief Add a NetworkProximityService to the simulation.
     *
     * @param service: a network proximity service
     *
     */
    void Simulation::addService(const std::shared_ptr<NetworkProximityService> &service) {
        if (service == nullptr) {
            throw std::invalid_argument("Simulation::addService(): invalid argument (nullptr service)");
        }
        service->simulation_ = this;
        this->network_proximity_services.insert(service);
    }

    /**
    * @brief Add a StorageService to the simulation.
    *
    * @param service: a storage service
    *
    */
    void Simulation::addService(const std::shared_ptr<StorageService> &service) {
        if (service == nullptr) {
            throw std::invalid_argument("Simulation::addService(): invalid argument (nullptr service)");
        }
        service->simulation_ = this;
        this->storage_services.insert(service);
    }

    /**
     * @brief Add an Execution Controller to the simulation.
     *
     * @param service: an execution controller
     *
     */
    void Simulation::addService(const std::shared_ptr<ExecutionController> &service) {
        if (service == nullptr) {
            throw std::invalid_argument("Simulation::addService(): invalid argument (nullptr service)");
        }
        service->simulation_ = this;
        this->execution_controllers.insert(service);
    }

    /**
      * @brief Add a FileRegistryService to the simulation.
      *
      * @param service: a file registry service
      *
      */
    void Simulation::addService(const std::shared_ptr<FileRegistryService> &service) {
        if (service == nullptr) {
            throw std::invalid_argument("Simulation::addService(): invalid argument (nullptr service)");
        }
        service->simulation_ = this;
        this->file_registry_services.insert(service);
    }

    /**
      * @brief Add an EnergyMeter service to the simulation.
      *
      * @param service: an energy meter service
      *
      */
    void Simulation::addService(const std::shared_ptr<EnergyMeterService> &service) {
        if (service == nullptr) {
            throw std::invalid_argument("Simulation::addService(): invalid argument (nullptr service)");
        }
        service->simulation_ = this;
        this->energy_meter_services.insert(service);
    }

    /**
      * @brief Add a BandwidthMeter service to the simulation.
      *
      * @param service: a link usage meter service
      *
      */
    void Simulation::addService(const std::shared_ptr<BandwidthMeterService> &service) {
        if (service == nullptr) {
            throw std::invalid_argument("Simulation::addService(): invalid argument (nullptr service)");
        }
        service->simulation_ = this;
        this->bandwidth_meter_services.insert(service);
    }

#ifdef PAGE_CACHE_SIMULATION
    /**
      * @brief Add a MemoryManager to the simulation.
      *
      * @param memory_manager: a MemoryManager
      *
      */
    void Simulation::addService(const std::shared_ptr<MemoryManager> &memory_manager) {
        if (memory_manager == nullptr) {
            throw std::invalid_argument("Simulation::addService(): invalid argument (nullptr memory_manager)");
        }
        memory_manager->simulation = this;
        this->memory_managers.insert(memory_manager);
    }
#endif

//    /**
//     * @brief Stage a copy of a file at a location (and add entries to all file registry services, if any)
//     * @param location: the file location
//     */
//    void Simulation::stageFile(const std::shared_ptr<FileLocation> &location) {
//
//        std::cerr << "CALLING STATEFILE " << location->getDirectoryPath() << "\n";
//        std::cerr << "CALLING STATEFILE " << location->getFilePath() << "\n";
//        if (location == nullptr) {
//            throw std::invalid_argument("Simulation::stageFile(): Invalid nullptr arguments");
//        }
//
//        if (std::dynamic_pointer_cast<CompoundStorageService>(location->getStorageService())) {
//            throw std::invalid_argument("Simulation::stageFile(): Can't stage on CompoundStorageService (it is only an abstraction layer)");
//        }
//
//        if (this->is_running) {
//            throw std::runtime_error(" Simulation::stageFile(): Cannot stage a file once the simulation has started");
//        }
//
//        // Put the file on the storage service (not via the service daemon)
//        try {
//            location->getStorageService()->createFile(location);
//        } catch (ExecutionException &e) {
//            throw std::invalid_argument("Simulation::stageFile(): Not enough space on disk");
//        }
//
//        // Update all file registry services (perhaps none)
//        for (const auto &frs: this->file_registry_services) {
//            frs->addEntryToDatabase(location);
//        }
//    }


    /**
     * @brief Method to create a new disk in the platform, which can be handy
     * @param hostname: the name of the host to which the disk should be attached
     * @param disk_id: the nae of the disk
     * @param read_bandwidth_in_bytes_per_sec: the disk's read bandwidth in byte/sec
     * @param write_bandwidth_in_bytes_per_sec: the disk's write bandwidth in byte/sec
     * @param capacity_in_bytes: the disk's capacity in bytes
     * @param mount_point: the disk's mount point (most people use "/")
     */
    void Simulation::createNewDisk(const std::string &hostname, const std::string &disk_id,
                                   double read_bandwidth_in_bytes_per_sec,
                                   double write_bandwidth_in_bytes_per_sec,
                                   sg_size_t capacity_in_bytes,
                                   const std::string &mount_point) {
        S4U_Simulation::createNewDisk(hostname, disk_id, read_bandwidth_in_bytes_per_sec, write_bandwidth_in_bytes_per_sec, capacity_in_bytes, mount_point);
    }

    /**
     * @brief Wrapper enabling timestamps for disk reads
     *
     * @param num_bytes: number of bytes read
     * @param hostname: hostname to read from
     * @param disk: disk to read from (nullptr if not known)
     *
     */
    void Simulation::readFromDisk(sg_size_t num_bytes, const std::string &hostname, simgrid::s4u::Disk *disk) {
        std::string mount_point = disk->get_property("mount");
        int unique_disk_sequence_number_read = this->getOutput().addTimestampDiskReadStart(Simulation::getCurrentSimulatedDate(), hostname, mount_point, num_bytes);
        try {
            S4U_Simulation::readFromDisk(num_bytes, hostname, mount_point, disk);
        } catch (const std::invalid_argument &) {
            this->getOutput().addTimestampDiskReadFailure(Simulation::getCurrentSimulatedDate(), hostname, mount_point, num_bytes,
                                                          unique_disk_sequence_number_read);
            throw;
        }
        this->getOutput().addTimestampDiskReadCompletion(Simulation::getCurrentSimulatedDate(), hostname, mount_point, num_bytes, unique_disk_sequence_number_read);
    }

    /**
     * @brief Wrapper enabling timestamps for concurrent disk read/writes
     *
     * @param num_bytes_to_read - number of bytes read
     * @param num_bytes_to_write - number of bytes written
     * @param hostname - hostname where disk is located
     * @param src_disk: source disk (nullptr if not known)
     * @param dst_disk: dst disk (nullptr if not known)
     *
     */
    void Simulation::readFromDiskAndWriteToDiskConcurrently(sg_size_t num_bytes_to_read, sg_size_t num_bytes_to_write,
                                                            const std::string &hostname,
                                                            simgrid::s4u::Disk *src_disk,
                                                            simgrid::s4u::Disk *dst_disk) {
        std::string read_mount_point = src_disk->get_property("mount");
        std::string write_mount_point = dst_disk->get_property("mount");

        int unique_disk_sequence_number_read = this->getOutput().addTimestampDiskReadStart(Simulation::getCurrentSimulatedDate(), hostname, read_mount_point, num_bytes_to_read);
        int unique_disk_sequence_number_write = this->getOutput().addTimestampDiskWriteStart(Simulation::getCurrentSimulatedDate(), hostname, write_mount_point, num_bytes_to_write);
        try {
            S4U_Simulation::readFromDiskAndWriteToDiskConcurrently(num_bytes_to_read, num_bytes_to_write, hostname,
                                                                   read_mount_point, write_mount_point, src_disk, dst_disk);
        } catch (const std::invalid_argument &) {
            this->getOutput().addTimestampDiskWriteFailure(Simulation::getCurrentSimulatedDate(), hostname, write_mount_point, num_bytes_to_write,
                                                           unique_disk_sequence_number_write);
            this->getOutput().addTimestampDiskReadFailure(Simulation::getCurrentSimulatedDate(), hostname, read_mount_point, num_bytes_to_read,
                                                          unique_disk_sequence_number_read);
            throw;
        }
        this->getOutput().addTimestampDiskWriteCompletion(Simulation::getCurrentSimulatedDate(), hostname, write_mount_point, num_bytes_to_write,
                                                          unique_disk_sequence_number_write);
        this->getOutput().addTimestampDiskReadCompletion(Simulation::getCurrentSimulatedDate(), hostname, read_mount_point, num_bytes_to_read,
                                                         unique_disk_sequence_number_read);
    }

    /**
     * @brief Wrapper enabling timestamps for disk writes
     *
     * @param num_bytes: number of bytes written
     * @param hostname: name of the host to write to
     * @param disk: a simgrid disk to write to (nullptr if not known)
     *
     */
    void Simulation::writeToDisk(sg_size_t num_bytes, const std::string &hostname, simgrid::s4u::Disk *disk) {
        std::string mount_point = disk->get_property("mount");
        int unique_disk_sequence_number_write = this->getOutput().addTimestampDiskWriteStart(Simulation::getCurrentSimulatedDate(), hostname, mount_point, num_bytes);
        try {
            S4U_Simulation::writeToDisk(num_bytes, hostname, mount_point, disk);
        } catch (const std::invalid_argument &) {
            this->getOutput().addTimestampDiskWriteFailure(Simulation::getCurrentSimulatedDate(), hostname, mount_point, num_bytes,
                                                           unique_disk_sequence_number_write);
            throw;
        }
        this->getOutput().addTimestampDiskWriteCompletion(Simulation::getCurrentSimulatedDate(), hostname, mount_point, num_bytes,
                                                          unique_disk_sequence_number_write);
    }

#ifdef PAGE_CACHE_SIMULATION
    /**
     * @brief Read file locally, only available if writeback is activated.
     *
     * @param file: workflow file
     * @param n_bytes: number of read bytes
     * @param location: file location
     */
    void Simulation::readWithMemoryCache(const std::shared_ptr<DataFile> &file, double n_bytes, const std::shared_ptr<FileLocation> &location) {
        std::string hostname = getHostName();

        unique_disk_sequence_number += 1;
        int temp_unique_sequence_number = unique_disk_sequence_number;
        this->getOutput().addTimestampDiskReadStart(Simulation::getCurrentSimulatedDate(), hostname, location->getPath(), n_bytes,
                                                    temp_unique_sequence_number);

        auto mem_mng = getMemoryManagerByHost(hostname);
        std::vector<Block *> file_blocks = mem_mng->getCachedBlocks(file->getID());
        long cached_amt = 0;
        for (auto &file_block: file_blocks) {
            cached_amt += (long) (file_block->getSize());
        }

        double from_disk = std::min(n_bytes, file->getSize() - (double) cached_amt);
        double from_cache = n_bytes - from_disk;

        mem_mng->flush(n_bytes + from_disk - mem_mng->getFreeMemory() - mem_mng->getEvictableMemory(),
                       file->getID());
        mem_mng->evict(n_bytes + from_disk - mem_mng->getFreeMemory(), file->getID());
        if (from_disk > 0) {
            mem_mng->readToCache(file->getID(), location, from_disk, false);
        }

        if (from_cache > 0) {
            mem_mng->readChunkFromCache(file->getID(), from_cache);
        }

        //        Anonymous used by application
        mem_mng->useAnonymousMemory(n_bytes);

        this->getOutput().addTimestampDiskReadCompletion(Simulation::getCurrentSimulatedDate(), hostname, location->getPath(), n_bytes,
                                                         temp_unique_sequence_number);
    }

    /**
     * @brief Write a file locally with writeback strategy, only available if writeback is activated.
     *
     * @param file: workflow file
     * @param n_bytes: number of written bytes
     * @param location: file location
     * @param is_dirty: true or false
     */
    void
    Simulation::writebackWithMemoryCache(const std::shared_ptr<DataFile> &file, double n_bytes, const std::shared_ptr<FileLocation> &location,
                                         bool is_dirty) {
        std::string hostname = getHostName();

        unique_disk_sequence_number += 1;
        int temp_unique_sequence_number = unique_disk_sequence_number;

        this->getOutput().addTimestampDiskWriteStart(Simulation::getCurrentSimulatedDate(), hostname, location->getMountPoint(), n_bytes,
                                                     temp_unique_sequence_number);

        MemoryManager *mem_mng = this->getMemoryManagerByHost(hostname);

        double remaining_dirty = 0;
        if (is_dirty) {
            remaining_dirty = mem_mng->getDirtyRatio() * mem_mng->getAvailableMemory() - mem_mng->getDirty();
        } else {
            remaining_dirty = mem_mng->getAvailableMemory();
        }

        double mem_bw_amt = 0;

        // free write to cache without forced flushing
        if (remaining_dirty > 0) {
            mem_mng->evict(std::min(n_bytes, remaining_dirty) - mem_mng->getFreeMemory(), file->getID());
            mem_bw_amt = std::min(n_bytes, mem_mng->getFreeMemory());
            mem_mng->writebackToCache(file->getID(), location, mem_bw_amt, is_dirty);
        }

        double remaining = n_bytes - mem_bw_amt;
        // if dirty_ratio is reached, dirty data needs to be flushed to disk to write new data
        while (remaining > 0) {
            mem_mng->flush(remaining, "");
            mem_mng->evict(remaining - mem_mng->getFreeMemory(), file->getID());

            double to_cache = std::min(mem_mng->getFreeMemory(), remaining);
            mem_mng->writebackToCache(file->getID(), location, to_cache, is_dirty);
            remaining -= to_cache;
        }

        this->getOutput().addTimestampDiskWriteCompletion(Simulation::getCurrentSimulatedDate(), hostname, location->getPath(), n_bytes,
                                                          temp_unique_sequence_number);
    }

    /**
     * @brief Write-through a file locally, only available if writeback is activated.
     *
     * @param file: workflow file
     * @param n_bytes: number of written bytes
     * @param location: file location
     */
    void Simulation::writeThroughWithMemoryCache(const std::shared_ptr<DataFile> &file, double n_bytes,
                                                 const std::shared_ptr<FileLocation> &location) {
        std::string hostname = getHostName();

        unique_disk_sequence_number += 1;
        int temp_unique_sequence_number = unique_disk_sequence_number;
        this->getOutput().addTimestampDiskWriteStart(Simulation::getCurrentSimulatedDate(), hostname, location->getPath(), n_bytes,
                                                     temp_unique_sequence_number);

        MemoryManager *mem_mng = this->getMemoryManagerByHost(hostname);

        // Write to disk
        auto ss = std::dynamic_pointer_cast<SimpleStorageService>(location->getStorageService());
        if (!ss) {
            throw...
        }
        this->writeToDisk(n_bytes, hostname, location->getMountPoint());

        mem_mng->evict(n_bytes - mem_mng->getFreeMemory(), file->getID());
        mem_mng->addToCache(file->getID(), location, n_bytes, false);

        this->getOutput().addTimestampDiskWriteCompletion(Simulation::getCurrentSimulatedDate(), hostname, location->getMountPoint(), n_bytes,
                                                          temp_unique_sequence_number);
    }

    /**
     * @brief Find MemoryManager running on a host based on hostname
     *
     * @param hostname: name of the host
     * @return pointer to the memory manager running on the host (or nullptr)
     */
    MemoryManager *Simulation::getMemoryManagerByHost(const std::string &hostname) {
        for (const auto &ptr: this->memory_managers) {
            if (strcmp(ptr->getHostname().c_str(), hostname.c_str()) == 0) {
                return ptr.get();
            }
        }
        return nullptr;
    }
#endif

    /**
     * @brief Wrapper for S4U_Simulation hostExists()
     *
     * @param hostname - name of host being queried
     * @return boolean of existence
     */
    bool Simulation::doesHostExist(const std::string &hostname) {
        return S4U_Simulation::hostExists(hostname);
    }

    /**
     * @brief Wrapper for S4U_Simulation linkExists()
     *
     * @param link_name - name of link being queried
     * @return boolean of existence
     */
    bool Simulation::doesLinkExist(const std::string &link_name) {
        return S4U_Simulation::linkExists(link_name);
    }

    /**
     * @brief Get the current simulated date
     * @return a date
     */
    double Simulation::getCurrentSimulatedDate() {
        return S4U_Simulation::getClock();
    }

    /**
     * @brief Get the memory_manager_service capacity of a host given a hostname
     * @param hostname: the hostname
     * @return a memory_manager_service capacity in bytes
     */
    sg_size_t Simulation::getHostMemoryCapacity(const std::string &hostname) {
        return S4U_Simulation::getHostMemoryCapacity(hostname);
    }

    /**
    * @brief Get the number of cores of a host given a hostname
    * @param hostname: the hostname
    * @return a number of cores
    */
    unsigned long Simulation::getHostNumCores(const std::string &hostname) {
        return S4U_Simulation::getHostNumCores(hostname);
    }

    /**
     * @brief Get the flop rate of one core of a host given a hostname
     * @param hostname: the hostname
     * @return a flop rate (flop / sec)
     */
    double Simulation::getHostFlopRate(const std::string &hostname) {
        return S4U_Simulation::getHostFlopRate(hostname);
    }

    /**
     * @brief Determine if a host has a disk mounted at some mount point
     * @param hostname: the hostname
     * @param mount_point: the mount point
     * @return true or false
     */
    bool Simulation::hostHasMountPoint(const std::string &hostname, const std::string &mount_point) {
        return S4U_Simulation::hostHasMountPoint(hostname, mount_point);
    }

    /**
     * @brief Returns whether a host is on or not
     * @param hostname: the hostname
     * @return true or false
     */
    bool Simulation::isHostOn(const std::string &hostname) {
        return S4U_Simulation::isHostOn(hostname);
    }

    /**
     * @brief Turns off a host
     * @param hostname: the hostname
     */
    void Simulation::turnOffHost(const std::string &hostname) {
        S4U_Simulation::turnOffHost(hostname);
    }

    /**
     * @brief Turns on a host
     * @param hostname: the hostname
     */
    void Simulation::turnOnHost(const std::string &hostname) {
        S4U_Simulation::turnOnHost(hostname);
    }

    /**
     * @brief Returns whether a link is on or not
     * @param link_name: the link_name
     * @return true or false
     */
    bool Simulation::isLinkOn(const std::string &link_name) {
        return S4U_Simulation::isLinkOn(link_name);
    }

    /**
     * @brief Turns off a link
     * @param link_name: the link_name
     */
    void Simulation::turnOffLink(const std::string &link_name) {
        S4U_Simulation::turnOffLink(link_name);
    }

    /**
     * @brief Turns on a link
     * @param link_name: the link_name
     */
    void Simulation::turnOnLink(const std::string &link_name) {
        S4U_Simulation::turnOnLink(link_name);
    }

    /**
     * @brief Get the memory_manager_service capacity of the host on which the calling process is running
     * @return a memory_manager_service capacity in bytes
     */
    sg_size_t Simulation::getMemoryCapacity() {
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
     * @brief Simulates a multi-threaded computation
     * @param num_threads: the number of threads
     * @param thread_creation_overhead: the thread creation overhead in seconds
     * @param sequential_work: the sequential work (in flops)
     * @param parallel_per_thread_work: the parallel per thread work (in flops)
     */
    void Simulation::computeMultiThreaded(unsigned long num_threads,
                                          double thread_creation_overhead,
                                          double sequential_work,
                                          double parallel_per_thread_work) {
        S4U_Simulation::compute_multi_threaded(num_threads, thread_creation_overhead, sequential_work, parallel_per_thread_work);
    }

    /**
     * @brief Get the simulation output object
     * @return simulation output object
     */
    SimulationOutput &Simulation::getOutput() {
        return this->output;
    }

    /**
     * @brief Obtains the current energy consumption of a host
     * @param hostname: the host name
     * @return current energy consumption in joules
     */
    double Simulation::getEnergyConsumed(const std::string &hostname) {
        return this->getEnergyConsumed(hostname, false);
    }

    /**
     * @brief Obtains the current energy consumption of a host and will add SimulationTimestampEnergyConsumption to
     *        simulation output if can_record is set to true
     * @param hostname: the host name
     * @param record_as_time_stamp: bool signaling whether to record a SimulationTimestampEnergyConsumption object
     * @return current energy consumption in joules
     */
    double Simulation::getEnergyConsumed(const std::string &hostname, bool record_as_time_stamp) {
        if (hostname.empty()) {
            throw std::invalid_argument("Simulation::getEnergyConsumed() requires a valid hostname");
        }

        double const consumption = S4U_Simulation::getEnergyConsumedByHost(hostname);
        if (record_as_time_stamp) {
            this->getOutput().addTimestampEnergyConsumption(Simulation::getCurrentSimulatedDate(), hostname, consumption);
        }

        return consumption;
    }

    /**
    * @brief Obtains the current energy consumption of a host
    * @param hostnames: the list of hostnames
    * @return current energy consumption in joules for each host, as a map indexed by hostnames
    */
    std::map<std::string, double> Simulation::getEnergyConsumed(const std::vector<std::string> &hostnames) {
        return this->getEnergyConsumed(hostnames, false);
    }

    /**
    * @brief Obtains the current energy consumption of a host and will add SimulationTimestampEnergyConsumption to
    *          simulation output if can_record is set to true
    * @param hostnames: the list of hostnames
    * @param record_as_time_stamps: whether or not to record a SimulationTimestampEnergyConsumption object for each host when this method is called
    * @return current energy consumption in joules for each host, as a map indexed by hostnames
    */
    std::map<std::string, double>
    Simulation::getEnergyConsumed(const std::vector<std::string> &hostnames, bool record_as_time_stamps) {
        if (hostnames.empty()) {
            throw std::invalid_argument("Simulation::getEnergyConsumed() requires a valid hostname");
        }

        std::map<std::string, double> energy_consumptions;
        for (auto const &h: hostnames) {
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
        this->getOutput().addTimestampPstateSet(Simulation::getCurrentSimulatedDate(), hostname, pstate);
    }

    /**
     * @brief Get the total number of power states of a host
     * @param hostname: the host name
     * @return The number of power states available for the host (as specified in the platform xml description file)
     */
    int Simulation::getNumberOfPstates(const std::string &hostname) {
        return S4U_Simulation::getNumberofPstates(hostname);
    }

    /**
     * @brief Get the current power state of a host
     * @param hostname: the host name
     * @return The index of the current pstate of the host (as specified in the platform xml description file)
     */
    int Simulation::getCurrentPstate(const std::string &hostname) {
        return static_cast<int>(S4U_Simulation::getCurrentPstate(hostname));
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
     * @brief Starts a new compute service during execution (i.e., one that was not passed to Simulation::add() before
     *        Simulation::launch() was called). The simulation takes ownership of
     *        the reference and will call the destructor.
     * @param service: An instance of a service
     * @return A pointer to the service instance
     */
    std::shared_ptr<ComputeService> Simulation::startNewService(ComputeService *service) {
        if (service == nullptr) {
            throw std::invalid_argument("Simulation::startNewService(): invalid argument (nullptr service)");
        }

        if (not this->is_running) {
            throw std::runtime_error("Simulation::startNewService(): simulation is not running yet");
        }

        service->simulation_ = this;
        std::shared_ptr<ComputeService> shared_ptr = std::shared_ptr<ComputeService>(service);
        this->compute_services.insert(shared_ptr);
        shared_ptr->start(shared_ptr, true, false);// Daemonized, no auto-restart
        //        if (service->hasScratch()) {
        //            service->getScratch()->simulation = this;
        //            service->getScratch()->start(service->getScratchSharedPtr(), true, false);// Daemonized, no auto-restart
        //        }

        return shared_ptr;
    }

    /**
     * @brief Starts a new storage service during  execution (i.e., one that was not passed to Simulation::add() before
     *        Simulation::launch() was called). The simulation takes ownership of
     *        the reference and will call the destructor.
     * @param service: An instance of a service
     * @return A pointer to the service instance
     */
    std::shared_ptr<StorageService> Simulation::startNewService(StorageService *service) {
        if (service == nullptr) {
            throw std::invalid_argument("Simulation::startNewService(): invalid argument (nullptr service)");
        }

        if (not this->is_running) {
            throw std::runtime_error("Simulation::startNewService(): simulation is not running yet");
        }


        service->simulation_ = this;
        std::shared_ptr<StorageService> shared_ptr = std::shared_ptr<StorageService>(service);
        this->storage_services.insert(shared_ptr);
        shared_ptr->start(shared_ptr, true, false);// Daemonized, no auto-restart

        return shared_ptr;
    }

    /**
     * @brief Starts a new network proximity service during execution (i.e., one that was not passed to Simulation::add() before
     *        Simulation::launch() was called). The simulation takes ownership of
     *        the reference and will call the destructor.
     * @param service: An instance of a service
     * @return A pointer to the service instance
     */
    std::shared_ptr<NetworkProximityService> Simulation::startNewService(NetworkProximityService *service) {
        if (service == nullptr) {
            throw std::invalid_argument("Simulation::startNewService(): invalid argument (nullptr service)");
        }

        if (not this->is_running) {
            throw std::runtime_error("Simulation::startNewService(): simulation is not running yet");
        }

        service->simulation_ = this;
        std::shared_ptr<NetworkProximityService> shared_ptr = std::shared_ptr<NetworkProximityService>(service);
        this->network_proximity_services.insert(shared_ptr);
        shared_ptr->start(shared_ptr, true, false);// Daemonized, no auto-restart

        return shared_ptr;
    }

    /**
     * @brief Starts a new file registry service during execution (i.e., one that was not passed to Simulation::add() before
     *        Simulation::launch() was called). The simulation takes ownership of
     *        the reference and will call the destructor.
     * @param service: An instance of a service
     * @return A pointer to the service instance
     */
    std::shared_ptr<FileRegistryService> Simulation::startNewService(FileRegistryService *service) {
        if (service == nullptr) {
            throw std::invalid_argument("Simulation::startNewService(): invalid argument (nullptr service)");
        }

        if (not this->is_running) {
            throw std::runtime_error("Simulation::startNewService(): simulation is not running yet");
        }

        service->simulation_ = this;
        std::shared_ptr<FileRegistryService> shared_ptr = std::shared_ptr<FileRegistryService>(service);
        this->file_registry_services.insert(shared_ptr);
        shared_ptr->start(shared_ptr, true, false);// Daemonized, no auto-restart

        return shared_ptr;
    }

#ifdef PAGE_CACHE_SIMULATION
    /**
     * @brief Starts a new memory_manager_service manager service during execution (i.e., one that was not passed to Simulation::add() before
     *        Simulation::launch() was called). The simulation takes ownership of
     *        the reference and will call the destructor.
     * @param service: An instance of a service
     * @return A pointer to the service instance
     */
    std::shared_ptr<MemoryManager> Simulation::startNewService(MemoryManager *service) {
        if (service == nullptr) {
            throw std::invalid_argument("Simulation::startNewService(): invalid argument (nullptr service)");
        }

        if (not this->is_running) {
            throw std::runtime_error("Simulation::startNewService(): simulation is not running yet");
        }

        service->simulation_ = this;
        std::shared_ptr<MemoryManager> shared_ptr = std::shared_ptr<MemoryManager>(service);
        this->memory_managers.insert(shared_ptr);
        shared_ptr->start(shared_ptr, true, false);// Daemonized, no auto-restart

        return shared_ptr;
    }
#endif

    /**
     * @brief Checks that the platform is well-defined
     */
    void Simulation::platformSanityCheck() const
    {
        auto hostnames = wrench::Simulation::getHostnameList();

        // Check link bandwidths (should be >0)
        this->s4u_simulation->checkLinkBandwidths();

        // Check RAM Capacities
        for (auto const &h: hostnames) {
            S4U_Simulation::getHostMemoryCapacity(h);
        }

        // Check Disk Capacities
        for (auto const &h: hostnames) {
            auto disks = S4U_Simulation::getDisks(h);
            for (auto const &d: disks) {
                S4U_Simulation::getDiskCapacity(h, d);
            }
        }

        // Check Disk Capacities
        for (auto const &h: hostnames) {
            auto disks = S4U_Simulation::getDisks(h);
            for (auto const &d: disks) {
                S4U_Simulation::getDiskCapacity(h, d);
            }
        }

        // Check Disk Prefixness
        for (auto const &h: hostnames) {
            auto disks = S4U_Simulation::getDisks(h);
            for (unsigned int i = 0; i < disks.size(); i++) {
                for (unsigned int j = 0; j < disks.size(); j++) {
                    if (j == i) {
                        continue;
                    }
                    if (disks[i] == disks[j]) {
                        throw std::invalid_argument("Simulation::platformSanityCheck(): Host " + h +
                                                    " has two disks with the same mount point '" + disks[i] + "'");
                    }
                    if ((disks[j] != "/") and (disks[i] != "/") and
                        (FileLocation::properPathPrefix(FileLocation::sanitizePath(disks[i]), FileLocation::sanitizePath(disks[j])))) {
                        throw std::invalid_argument("Simulation::platformSanityCheck(): Host " + h +
                                                    " has two disks, with one of them having a mount point that "
                                                    "is a prefix of the other (" +
                                                    disks[j] + " and " + disks[i] + ")");
                    }
                }
            }
        }

        //        // Check Disk Bandwidth
        //        for (auto const &h: hostnames) {
        //            //            WRENCH_INFO("---> %s", h.c_str());
        //            for (auto const &d: simgrid::s4u::Host::by_name(h)->get_disks()) {
        //                double read_bw = d->get_read_bandwidth();
        //                double write_bw = d->get_write_bandwidth();
        //                if (std::abs<double>(read_bw - write_bw) > DBL_EPSILON) {
        //                    throw std::invalid_argument("Simulation::platformSanityCheck(): For now, disks must have equal "
        //                                                "read and write bandwidth (offending disk: " +
        //                                                h + ":" + d->get_property("mount"));
        //                }
        //            }
        //        }

        // Check that if --pagecache is passed, each host has a "/memory" disk
        if (this->isPageCachingEnabled()) {
            for (auto const &h: hostnames) {
                bool has_memory_disk = false;
                for (auto const &d: S4U_Simulation::get_host_or_vm_by_name(h)->get_disks()) {
                    if (std::string(d->get_property("mount")) == "/memory") {
                        has_memory_disk = true;
                        break;
                    }
                }
                if (not has_memory_disk) {
                    throw std::invalid_argument("Simulation::platformSanityCheck(): Since --pagecache was passed, "
                                                "each host must have a disk with mount point \"/memory\" (host " +
                                                h +
                                                " doesn't!)");
                }
            }
        }
    }

    /**
      * @brief Get the list of all files in the simulation
      *
      * @return a reference to the map of files in the simulation, indexed by file ID
      */
    std::map<std::string, std::shared_ptr<DataFile>> &Simulation::getFileMap() {
        return Simulation::data_files;
    }

    /**
    * @brief Find a DataFile based on its ID
    *
    * @param id: a string id
    *
    * @return the DataFile instance (or throws a std::invalid_argument if not found)
    *
    */
    std::shared_ptr<DataFile> Simulation::getFileByID(const std::string &id) {
        if (Simulation::data_files.find(id) == Simulation::data_files.end()) {
            throw std::invalid_argument("Workflow::getFileByID(): Unknown DataFile ID " + id);
        } else {
            return Simulation::data_files[id];
        }
    }

    /**
    * @brief Find a DataFile based on its ID
    *
    * @param id: a string id
    *
    * @return the DataFile instance (or nullptr if not found)
    *
    */
    std::shared_ptr<DataFile> Simulation::getFileByIDOrNull(const std::string &id) {
        if (Simulation::data_files.find(id) == Simulation::data_files.end()) {
            return nullptr;
        } else {
            return Simulation::data_files[id];
        }
    }

    /**
     * @brief Add a new file to the simulation (use at your own peril if you're using the workflow API - use Workflow::addFile() instead)
     *
     * @param id: a unique string id
     * @param size: a file size in bytes
     *
     * @return the DataFile instance
     *
     */
    std::shared_ptr<DataFile> Simulation::addFile(const std::string &id, sg_size_t size) {
        // Create the DataFile object
        if (Simulation::data_files.find(id) != Simulation::data_files.end()) {
            throw std::invalid_argument("Simulation::addFile(): DataFile with id '" +
                                        id + "' already exists");
        }

        auto file = std::shared_ptr<DataFile>(new DataFile(id, size));

        // Add if to the set of workflow files
        Simulation::data_files[file->id] = file;

        return file;
    }

    /**
     * @brief Add a new file to the simulation (use at your own peril if you're using the workflow API - use Workflow::addFile() instead)
     *
     * @param id: a unique string id
     * @param size: a size as a unit string (e.g., "10MB")
     *
     * @return the DataFile instance
     *
     */
    std::shared_ptr<DataFile> Simulation::addFile(const std::string &id, const std::string &size) {
        return Simulation::addFile(id, UnitParser::parse_size(size));
    }

    /**
     * @brief Remove a file from the simulation (use at your own peril if you're using the workflow API - use Workflow::removeFile() instead)
     * @param file : file to remove
     */
    void Simulation::removeFile(const std::shared_ptr<DataFile> &file) {
        if (Simulation::data_files.find(file->getID()) == Simulation::data_files.end()) {
            throw std::invalid_argument("Simulation::removeFile(): Unknown file");
        }
        Simulation::data_files.erase(file->getID());
    }

    /**
     * @brief Remove all files from the simulation (use at your own peril if you're using the workflow API)
     */
    void Simulation::removeAllFiles() {
        Simulation::data_files.clear();
    }


    /**
     * @brief Determine if the simulation has been initialized
     * @return  true or false
     */
    bool Simulation::isInitialized() {
        return Simulation::initialized;
    }

    /**
     * @brief Create a simulation
     * @return a simulation
     */
    std::shared_ptr<Simulation> Simulation::createSimulation() {
//        LogicalFileSystem::mount_points.clear();
        return std::shared_ptr<Simulation>(new Simulation());
    }

}// namespace wrench
