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
#include <wrench/services/compute/bare_metal/BareMetalComputeServiceOneShot.h>

#include "Version.h"
#include "wrench/simulation/SimulationOutput.h"


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
    class ExecutionController;
    class DataFile;
    class SimulationOutput;
    class S4U_Simulation;
    class FileLocation;
#ifdef PAGE_CACHE_SIMULATION
    class MemoryManager;
#endif

    /**
     * @brief A class that provides basic simulation methods.  Once the simulation object has been
     *        explicitly or implicitly destroyed, then any call to the WRENCH APIs has undefied behavior (due
     *        to memory_manager_service being de-allocated).
     */
    class Simulation {
    public:
        //        void printRefCounts(std::string message) {
        //            std::cerr << message << " SIMUALTION: CS\n";
        //            for (auto const &cs : this->compute_services) {
        //                std::cerr  << "    CS REFCOUNT: " << cs.use_count() - 1 << "\n";
        //            }
        //            std::cerr << message << " SIMUALTION: SS\n";
        //            for (auto const &cs : this->storage_services) {
        //                std::cerr << "    SS REFCOUNT: " << cs.use_count() - 1 << "\n";
        //            }
        //        }

        static std::shared_ptr<Simulation> createSimulation();

        ~Simulation();

        void init(int *, char **);

        static bool isInitialized();

        void instantiatePlatform(const std::string &);
        void instantiatePlatform(const std::function<void()> &);

        static std::vector<std::string> getHostnameList();
        static std::map<std::string, std::vector<std::string>> getHostnameListByCluster();
        static double getHostMemoryCapacity(const std::string &hostname);
        static unsigned long getHostNumCores(const std::string &hostname);
        static double getHostFlopRate(const std::string &hostname);
        static bool hostHasMountPoint(const std::string &hostname, const std::string &scratch_space_mount_point);

        static std::map<std::string, std::shared_ptr<DataFile>> &getFileMap();
        static void removeFile(const std::shared_ptr<DataFile> &file);
        static void removeAllFiles();
        static std::shared_ptr<DataFile> getFileByID(const std::string &id);
        static std::shared_ptr<DataFile> addFile(const std::string &, double);


        void launch();

        /**
         * @brief Method to add a service to the simulation
         * @tparam T: The service class (base class is Service)
         * @param t: the service object
         * @return a shared_ptr to the service object
         */
        template<class T>
        std::shared_ptr<T> add(T *t) {
            auto s = std::shared_ptr<T>(t);
            this->addService(s);
            return s;
        }

        /**
         * @brief Method to add a service to the simulation
         * @tparam T: The service class (base class is Service)
         * @param t: the service object (shared ptr)
         * @return a shared_ptr to the service object
         */
        template<class T>
        std::shared_ptr<T> add(std::shared_ptr<T> t) {
            this->addService(t);
            return t;
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

        /**
	 * @brief Creates a file copy on a storage service before the simulation begins
	 * @param file: a file
	 * @param storage_service: a storage service
	 */
        void stageFile(const std::shared_ptr<DataFile> file, const std::shared_ptr<StorageService> &storage_service) {
            this->stageFile(wrench::FileLocation::LOCATION(storage_service, file));
        }
        /**
         * @brief Creates a file copy on a storage service before the simulation begins
         * @param file: a file
         * @param storage_service: a storage service
	 * @param path: a path
         */
        void stageFile(const std::shared_ptr<DataFile> file, const std::shared_ptr<StorageService> &storage_service, const std::string &path) {
            this->stageFile(wrench::FileLocation::LOCATION(storage_service, path, file));
        }

        void stageFile(const std::shared_ptr<FileLocation> &location);

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/
        double getEnergyConsumed(const std::string &hostname, bool record_as_time_stamp);

        static std::vector<std::string> getRoute(std::string &src_host, std::string &dst_host);

        double getLinkUsage(const std::string &link_name, bool record_as_time_stamp);
        std::map<std::string, double> getEnergyConsumed(const std::vector<std::string> &hostnames, bool record_as_time_stamps);

        static bool doesHostExist(const std::string &hostname);
        static bool isHostOn(const std::string &hostname);
        static void turnOnHost(const std::string &hostname);
        static void turnOffHost(const std::string &hostname);
        static bool doesLinkExist(const std::string &link_name);
        static bool isLinkOn(const std::string &link_name);
        static void turnOnLink(const std::string &link_name);
        static void turnOffLink(const std::string &link_name);

        static void createNewDisk(const std::string &hostname, const std::string &disk_id,
                                  double read_bandwidth_in_bytes_per_sec,
                                  double write_bandwidth_in_bytes_per_sec,
                                  double capacity_in_bytes,
                                  const std::string &mount_point);

        // pstate related calls
        void setPstate(const std::string &hostname, int pstate);
        static int getCurrentPstate(const std::string &hostname);

        std::shared_ptr<ComputeService> startNewService(ComputeService *service);
        std::shared_ptr<StorageService> startNewService(StorageService *service);
        std::shared_ptr<NetworkProximityService> startNewService(NetworkProximityService *service);
        std::shared_ptr<FileRegistryService> startNewService(FileRegistryService *service);
#ifdef PAGE_CACHE_SIMULATION
        std::shared_ptr<MemoryManager> startNewService(MemoryManager *service);
#endif

        static double getCurrentSimulatedDate();

        static void sleep(double duration);
        static void compute(double flops);
        static void computeMultiThreaded(unsigned long num_threads,
                                         double thread_creation_overhead,
                                         double sequential_work,
                                         double parallel_per_thread_work);

        /***********************/
        /** \endcond           */
        /***********************/

        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        void readFromDisk(double num_bytes, const std::string &hostname, const std::string &mount_point, simgrid::s4u::Disk *disk);
        void readFromDiskAndWriteToDiskConcurrently(double num_bytes_to_read, double num_bytes_to_write,
                                                    const std::string &hostname,
                                                    const std::string &read_mount_point,
                                                    const std::string &write_mount_point,
                                                    simgrid::s4u::Disk *src_disk,
                                                    simgrid::s4u::Disk *dst_disk);
        void writeToDisk(double num_bytes, const std::string &hostname, const std::string &mount_point, simgrid::s4u::Disk *disk);

#ifdef PAGE_CACHE_SIMULATION
        void readWithMemoryCache(const std::shared_ptr<DataFile> &file, double n_bytes, const std::shared_ptr<FileLocation> &location);
        void writebackWithMemoryCache(const std::shared_ptr<DataFile> &file, double n_bytes, const std::shared_ptr<FileLocation> &location, bool is_dirty);
        void writeThroughWithMemoryCache(const std::shared_ptr<DataFile> &file, double n_bytes, const std::shared_ptr<FileLocation> &location);
        MemoryManager *getMemoryManagerByHost(const std::string &hostname);
#endif

        static double getMemoryCapacity();
        static unsigned long getNumCores();
        static double getFlopRate();
        static std::string getHostName();

        static std::vector<std::string> getLinknameList();
        static double getLinkUsage(const std::string &link_name);
        static double getLinkBandwidth(const std::string &link_name);
        static bool isPageCachingEnabled();
        static bool isHostShutdownSimulationEnabled();
        static bool isLinkShutdownSimulationEnabled();
        static bool isEnergySimulationEnabled();
        static bool isSurfPrecisionSetByUser();

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        Simulation();

        SimulationOutput output;

        std::unique_ptr<S4U_Simulation> s4u_simulation;

        std::set<std::shared_ptr<ExecutionController>> execution_controllers;
        std::set<std::shared_ptr<FileRegistryService>> file_registry_services;
        std::set<std::shared_ptr<EnergyMeterService>> energy_meter_services;
        std::set<std::shared_ptr<BandwidthMeterService>> bandwidth_meter_services;
        std::set<std::shared_ptr<NetworkProximityService>> network_proximity_services;
        std::set<std::shared_ptr<ComputeService>> compute_services;
        std::set<std::shared_ptr<StorageService>> storage_services;

#ifdef PAGE_CACHE_SIMULATION
        std::set<std::shared_ptr<MemoryManager>> memory_managers;
#endif

        static int unique_disk_sequence_number;


        void platformSanityCheck();
        void checkSimulationSetup();
        //        bool isRunning() const;

        void startAllProcesses();
        void addService(const std::shared_ptr<ComputeService> &service);
        void addService(const std::shared_ptr<StorageService> &service);
        void addService(const std::shared_ptr<NetworkProximityService> &service);
        void addService(const std::shared_ptr<ExecutionController> &service);
        void addService(const std::shared_ptr<FileRegistryService> &service);
        void addService(const std::shared_ptr<EnergyMeterService> &service);
        void addService(const std::shared_ptr<BandwidthMeterService> &service);

#ifdef PAGE_CACHE_SIMULATION
        void addService(const std::shared_ptr<MemoryManager> &memory_manager);
#endif

        static std::string getWRENCHVersionString() { return WRENCH_VERSION_STRING; }

        bool is_running = false;

        bool already_setup = false;

        static bool energy_enabled;
        static bool host_shutdown_enabled;
        static bool link_shutdown_enabled;
        static bool pagecache_enabled;

        static bool initialized;

        static bool surf_precision_set_by_user;

        /* Map of files */
        static std::map<std::string, std::shared_ptr<DataFile>> data_files;
    };


}// namespace wrench

#endif//WRENCH_SIMULATION_H
