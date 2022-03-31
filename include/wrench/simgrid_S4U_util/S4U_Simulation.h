/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_S4U_SIMULATION_H
#define WRENCH_S4U_SIMULATION_H

#include <set>
#include <simgrid/s4u.hpp>
#include <simgrid/kernel/routing/ClusterZone.hpp>

namespace wrench {


    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Wrappers around S4U's basic simulation methods
     */
    class S4U_Simulation {

    public:
        /** @brief The ram capacity of a physical host whenever not specified in the platform description file */
        static constexpr double DEFAULT_RAM = (1024.0 * 1024.0 * 1024.0 * 1024.0 * 1024.0);// 1 PiB

    public:
        void initialize(int *argc, char **argv);
        void setupPlatform(std::string &);
        void setupPlatform(const std::function<void()> &creation_function);
        void runSimulation();
        static double getClock();
        static std::string getHostName();
        static bool hostExists(std::string hostname);
        static bool linkExists(std::string linkname);
        static std::vector<std::string> getRoute(std::string &src_host, std::string &dst_host);
        static unsigned int getHostNumCores(std::string hostname);
        static unsigned int getNumCores();
        static double getHostFlopRate(std::string hostname);
        static bool isHostOn(std::string hostname);
        static void turnOffHost(std::string hostname);
        static void turnOnHost(std::string hostname);
        static bool isLinkOn(std::string linkname);
        static void turnOffLink(std::string linkname);
        static void turnOnLink(std::string linkname);
        static double getFlopRate();
        static double getHostMemoryCapacity(std::string hostname);
        static double getMemoryCapacity();
        static void compute(double);
        static void compute_multi_threaded(unsigned long num_threads,
                                           double thread_creation_overhead,
                                           double sequential_work,
                                           double parallel_per_thread_work);
        static void sleep(double);
        static void computeZeroFlop();
        static void writeToDisk(double num_bytes, std::string hostname, std::string mount_point);
        static void readFromDisk(double num_bytes, std::string hostname, std::string mount_point);
        static void readFromDiskAndWriteToDiskConcurrently(double num_bytes_to_read, double num_bytes_to_write,
                                                           std::string hostname,
                                                           std::string read_mount_point,
                                                           std::string write_mount_point);

        static double getDiskCapacity(std::string hostname, std::string mount_point);
        static std::vector<std::string> getDisks(std::string hostname);
        static bool hostHasMountPoint(std::string hostname, std::string mount_point);

        void checkLinkBandwidths();

        static void yield();
        static std::string getHostProperty(std::string hostname, std::string property_name);
        static void setHostProperty(std::string hostname, std::string property_name, std::string property_value);
        static std::string getClusterProperty(std::string cluster_id, std::string property_name);

        //start energy related calls
        static double getEnergyConsumedByHost(const std::string &hostname);
        //		static double getTotalEnergyConsumed(const std::vector<std::string> &hostnames);
        static void setPstate(const std::string &hostname, unsigned long pstate);
        static int getNumberofPstates(const std::string &hostname);
        static unsigned long getCurrentPstate(const std::string &hostname);
        static double getMinPowerConsumption(const std::string &hostname);
        static double getMaxPowerConsumption(const std::string &hostname);
        static std::vector<int> getListOfPstates(const std::string &hostname);
        //end energy related calls

        bool isInitialized();
        bool isPlatformSetup();
        static std::vector<std::string> getAllHostnames();
        static std::vector<std::string> getAllLinknames();
        static double getLinkBandwidth(std::string name);
        static double getLinkUsage(std::string name);

        static std::map<std::string, std::vector<std::string>> getAllHostnamesByCluster();
        static std::map<std::string, std::vector<std::string>> getAllHostnamesByZone();
        static std::map<std::string, std::vector<std::string>> getAllClusterIDsByZone();
        static std::map<std::string, std::vector<std::string>> getAllSubZoneIDsByZone();

        void shutdown();

    private:

        static void traverseAllNetZonesRecursive(simgrid::s4u::NetZone *nz, std::map<std::string, std::vector<std::string>> &result, bool get_subzones, bool get_clusters, bool get_hosts_from_zones, bool get_hosts_from_clusters);

        static double getHostMemoryCapacity(simgrid::s4u::Host *host);
        simgrid::s4u::Engine *engine;
        bool initialized = false;
        bool platform_setup = false;
    };

    /***********************/
    /** \endcond           */
    /***********************/
};// namespace wrench

#endif//WRENCH_S4U_SIMULATION_H
