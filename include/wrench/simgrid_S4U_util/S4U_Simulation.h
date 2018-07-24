/**
 * Copyright (c) 2017. The WRENCH Team.
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
				void initialize(int *argc, char **argv);
				void setupPlatform(std::string&);
				void runSimulation();
				static double getClock();
				static std::string getHostName();
				bool hostExists(std::string hostname);
				static unsigned int getHostNumCores(std::string hostname);
				static double getHostFlopRate(std::string hostname);
				static double getHostMemoryCapacity(std::string hostname);
				static double getMemoryCapacity();
				static void compute(double);
				static void sleep(double);

				//start enery related calls
				static std::string getHostProperty(std::string hostname, std::string property_name);
				static double getEnergyConsumedByHost(std::string hostname);
				static double getTotalEnergyConsumed(std::vector<std::string> hostnames);
				static void setPstate(std::string hostname, int pstate);
				static int getNumberofPstates(std::string hostname);
				static int getCurrentPstate(std::string hostname);
				static double getMinPowerAvailable(std::string hostname);
				static double getMaxPowerPossible(std::string hostname);
				std::vector<int> getListOfPstates(std::string hostname);
				//end energy related calls

				bool isInitialized();
        bool isPlatformSetup();
        std::vector<std::string> getAllHostnames();
        std::map<std::string, std::vector<std::string>> getAllHostnamesByCluster();
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
