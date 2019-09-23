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
        static constexpr double DEFAULT_RAM = (1024.0 * 1024.0 * 1024.0 * 1024.0 * 1024.0); // 1 PiB

    public:
        void initialize(int *argc, char **argv);
        void setupPlatform(std::string&);
        void runSimulation();
        static double getClock();
        static std::string getHostName();
        static bool hostExists(std::string hostname);
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
        static void sleep(double);
        static void computeZeroFlop();
        static void writeToDisk(double num_bytes, std::string hostname, std::string diskname);
        static void readFromDisk(double num_bytes, std::string hostname, std::string diskname);
        static double getDiskCapacity(std::string hostname, std::string mount_point);
        std::set<std::string> getDisks(std::string hostname);
        static bool hostHasDisk(std::string hostname, std::string mount_point);

        static void yield();
        static std::string getHostProperty(std::string hostname, std::string property_name);

        //start energy related calls
        static double getEnergyConsumedByHost(const std::string &hostname);
//		static double getTotalEnergyConsumed(const std::vector<std::string> &hostnames);
        static void setPstate(const std::string &hostname, int pstate);
        static int getNumberofPstates(const std::string &hostname);
        static int getCurrentPstate(const std::string &hostname);
        static double getMinPowerConsumption(const std::string &hostname);
        static double getMaxPowerConsumption(const std::string &hostname);
        static std::vector<int> getListOfPstates(const std::string &hostname);
        //end energy related calls

        bool isInitialized();
        bool isPlatformSetup();
        static std::vector<std::string> getAllHostnames();
        static std::map<std::string, std::vector<std::string>> getAllHostnamesByCluster();
        void shutdown();

    private:
        static double getHostMemoryCapacity(simgrid::s4u::Host *host);
        simgrid::s4u::Engine *engine;
        bool initialized = false;
        bool platform_setup = false;

    };

    /***********************/
    /** \endcond           */
    /***********************/
};

#endif //WRENCH_S4U_SIMULATION_H
