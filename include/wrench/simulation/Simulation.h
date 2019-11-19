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
    class Service;
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
    class FileLocation;


    /**
     * @brief A class that provides basic simulation methods.  Once the simulation object has been
     *        explicitly or implicitly destroyed, then any call to the WRENCH APIs has undefied behavior (due
     *        to memory being de-allocated).
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

        void launch();

        /**
         * @brief Method to add a service to the simulation
         * @tparam T: The service class (base class is Service)
         * @param t: the service object
         * @return a shared_ptr to the service object
         */
        template <class T>
        std::shared_ptr<T> add(T *t) {
            auto s = std::shared_ptr<T>(t);
            this->addService(s);
            return s;
        }


        SimulationOutput &getOutput();

        //start energy related calls
        double getEnergyConsumed(const std::string &hostname, bool record_as_time_stamp = false);
        std::map<std::string, double> getEnergyConsumed(const std::vector<std::string> &hostnames, bool record_as_time_stamps = false);
//        double getEnergyTimestamp(const std::string &hostname, bool can_record = false);

        // pstate related calls
        static int getNumberofPstates(const std::string &hostname);
        static double getMinPowerConsumption(const std::string &hostname);
        static double getMaxPowerConsumption(const std::string &hostname);
        static std::vector<int> getListOfPstates(const std::string &hostname);

        void stageFile(WorkflowFile *file, std::shared_ptr<StorageService> ss);
        void stageFile(WorkflowFile *file, std::shared_ptr<StorageService> ss, std::string directory_absolute_path);


        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        static bool isHostOn(std::string hostname);
        static void turnOnHost(std::string hostname);
        static void turnOffHost(std::string hostname);
        static bool isLinkOn(std::string linkname);
        static void turnOnLink(std::string linkname);
        static void turnOffLink(std::string linkname);

        // pstate related calls
        void setPstate(const std::string &hostname, int pstate);
        static int getCurrentPstate(const std::string &hostname);


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

        void stageFile(WorkflowFile *file, std::shared_ptr<FileLocation> location);


        void platformSanityCheck();
        void checkSimulationSetup();
        bool isRunning();

        void startAllProcesses();
        void addService(std::shared_ptr<ComputeService> service);
        void addService(std::shared_ptr<StorageService> service);
        void addService(std::shared_ptr<NetworkProximityService> service);
        void addService(std::shared_ptr<WMS> service);
        void addService(std::shared_ptr<FileRegistryService> service);

        std::string getWRENCHVersionString() { return WRENCH_VERSION_STRING; }

        bool is_running = false;

        unsigned int on_state_change_callback_id;


    };

};

#endif //WRENCH_SIMULATION_H
