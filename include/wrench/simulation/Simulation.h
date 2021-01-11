/**
 * Copyright (c) 2017-2020. The WRENCH Team.
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
    class EnergyMeterService;
    class BandwidthMeterService;
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
    class MemoryManager;

    /**
     * @brief A class that provides basic simulation methods.  Once the simulation object has been
     *        explicitly or implicitly destroyed, then any call to the WRENCH APIs has undefied behavior (due
     *        to memory_manager_service being de-allocated).
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
        double getEnergyConsumed(const std::string &hostname);
        std::map<std::string, double> getEnergyConsumed(const std::vector<std::string> &hostnames);
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
        double getEnergyConsumed(const std::string &hostname, bool record_as_time_stamp);

        static std::vector<std::string> getRoute(std::string src_host, std::string dst_host);

        double getLinkUsage(const std::string &linkname, bool  record_as_time_stamp);
        std::map<std::string, double> getEnergyConsumed(const std::vector<std::string> &hostnames, bool record_as_time_stamps);

        static bool doesHostExist(std::string hostname);
        static bool isHostOn(std::string hostname);
        static void turnOnHost(std::string hostname);
        static void turnOffHost(std::string hostname);
        static bool doesLinkExist(std::string linkname);
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
        std::shared_ptr<MemoryManager> startNewService(MemoryManager *service);

        static double getCurrentSimulatedDate();

        static void sleep(double duration);
        static void compute(double flops);
        /***********************/
        /** \endcond           */
        /***********************/

        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        void readFromDisk(double num_bytes, std::string hostname, std::string mount_point);
        void readFromDiskAndWriteToDiskConcurrently(double num_bytes_to_read, double num_bytes_to_write,
                                                    std::string hostname,
                                                    std::string read_mount_point,
                                                    std::string write_mount_point);
        void writeToDisk(double num_bytes, std::string hostname, std::string mount_point);

        void readWithMemoryCache(WorkflowFile *file, double n_bytes, std::shared_ptr<FileLocation> location);
        void writebackWithMemoryCache(WorkflowFile *file, double n_bytes, std::shared_ptr<FileLocation> location, bool is_dirty);
        void writeThroughWithMemoryCache(WorkflowFile *file, double n_bytes, std::shared_ptr<FileLocation> location);
        MemoryManager* getMemoryManagerByHost(std::string hostname);

        static double getMemoryCapacity();
        static unsigned long getNumCores();
        static double getFlopRate();
        static std::string getHostName();

        static std::vector<std::string> getLinknameList();
        static double getLinkUsage(std::string linkname);
        static double getLinkBandwidth(std::string linkname);
        static bool isPageCachingEnabled();

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        SimulationOutput output;

        std::unique_ptr<S4U_Simulation> s4u_simulation;

        std::set<std::shared_ptr<WMS>> wmses;

        std::set<std::shared_ptr<FileRegistryService>> file_registry_services;

        std::set<std::shared_ptr<EnergyMeterService>> energy_meter_services;

        std::set<std::shared_ptr<BandwidthMeterService>> bandwidth_meter_services;

        std::set<std::shared_ptr<NetworkProximityService>> network_proximity_services;

        std::set<std::shared_ptr<ComputeService>> compute_services;

        std::set<std::shared_ptr<StorageService>> storage_services;

        std::set<std::shared_ptr<MemoryManager>> memory_managers;

        static int unique_disk_sequence_number;

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
        void addService(std::shared_ptr<EnergyMeterService> service);
        void addService(std::shared_ptr<BandwidthMeterService> service);
        void addService(std::shared_ptr<MemoryManager> memory_manager);

        std::string getWRENCHVersionString() { return WRENCH_VERSION_STRING; }

        bool is_running = false;

        unsigned int on_state_change_callback_id;

        static bool pagecache_enabled;
    };

};

#endif //WRENCH_SIMULATION_H
