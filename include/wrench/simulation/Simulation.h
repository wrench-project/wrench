/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_SIMULATION_H
#define WRENCH_SIMULATION_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "Version.h"

#include <wrench/simulation/SimulationOutput.h>


namespace wrench {

    class StorageService;
    class NetworkProximityService;
    class FileRegistryService;
    class ComputeService;
    class BatchComputeService;
    class BareMetalComputeService;
    class CloudComputeService;
    class VirtualizedClusterComputeService;
    class WMS;
    class WorkflowFile;
    class SimulationOutput;
    class S4U_Simulation;


    /**
     * @brief A class that provides basic simulation methods
     */
    class Simulation {

    public:
        Simulation();

        ~Simulation();

        void init(int *, char **);

        void instantiatePlatform(std::string);

        static std::vector<std::string> getHostnameList();
        static std::map<std::string, std::vector<std::string>> getHostnameListByCluster();
        static double getHostMemoryCapacity(std::string hostname);
        static unsigned long getHostNumCores(std::string hostname);
        static double getHostFlopRate(std::string hostname);
        static bool isHostOn(std::string hostname);

        void launch();

        std::shared_ptr<ComputeService> add(ComputeService *);
        std::shared_ptr<StorageService> add(StorageService *);
        std::shared_ptr<NetworkProximityService> add(NetworkProximityService *);
        std::shared_ptr<WMS> add(WMS *);
        std::shared_ptr<FileRegistryService> add(FileRegistryService *);

        void stageFile(WorkflowFile *file, std::shared_ptr<StorageService> storage_service);
        void stageFile(WorkflowFile *file, std::shared_ptr<StorageService> storage_service, std::string partition);

        void stageFiles(std::map<std::string, WorkflowFile *> files, std::shared_ptr<StorageService>storage_service);
        void stageFiles(std::map<std::string, WorkflowFile *> files, std::shared_ptr<StorageService>storage_service, std::string partition);

        SimulationOutput &getOutput();

        //start energy related calls
        double getEnergyConsumed(const std::string &hostname, bool record_as_time_stamp = false);
        std::map<std::string, double> getEnergyConsumed(const std::vector<std::string> &hostnames, bool record_as_time_stamps = false);
//        double getEnergyTimestamp(const std::string &hostname, bool can_record = false);

        // pstate related calls
        void setPstate(const std::string &hostname, int pstate);
        static int getNumberofPstates(const std::string &hostname);
        static int getCurrentPstate(const std::string &hostname);
        static double getMinPowerConsumption(const std::string &hostname);
        static double getMaxPowerConsumption(const std::string &hostname);
        static std::vector<int> getListOfPstates(const std::string &hostname);


        /***********************/
        /** \cond DEVELOPER    */
        /***********************/


        std::shared_ptr<ComputeService> startNewService(ComputeService *service);
        std::shared_ptr<StorageService> startNewService(StorageService *service);
        std::shared_ptr<NetworkProximityService> startNewService(NetworkProximityService *service);
        std::shared_ptr<FileRegistryService> startNewService(FileRegistryService *service);


        static double getMemoryCapacity();
        static unsigned long getNumCores();
        static double getFlopRate();
        static std::string getHostName();

        static double getCurrentSimulatedDate();



        static void sleep(double duration);
        static void compute(double flops);

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        SimulationOutput output;

        std::unique_ptr<S4U_Simulation> s4u_simulation;

        std::set<std::shared_ptr<WMS>> wmses;

        std::set<std::shared_ptr<FileRegistryService>> file_registry_services;

        std::set<std::shared_ptr<NetworkProximityService>> network_proximity_services;

        std::set<std::shared_ptr<ComputeService>> compute_services;

        std::set<std::shared_ptr<StorageService>> storage_services;

        void checkSimulationSetup();

        void startAllProcesses();

        std::string getWRENCHVersionString() { return WRENCH_VERSION_STRING; }

        bool is_running = false;

        unsigned int on_state_change_callback_id;


        };

};

#endif //WRENCH_SIMULATION_H
