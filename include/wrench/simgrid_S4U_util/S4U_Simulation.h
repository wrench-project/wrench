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

namespace wrench {

		/***********************/
		/** \cond INTERNAL     */
		/***********************/

		/**
		 * @brief Wrappers around Simgrid's basic simulation methods
		 */
		class S4U_Simulation {

		public:
				void initialize(int *argc, char **argv);
				void setupPlatform(std::string&);
				void runSimulation();
				static double getClock();
				static std::string getHostName();
				bool hostExists(std::string hostname);
				static unsigned int getNumCores(std::string hostname);
				static double getFlopRate(std::string hostname);
				static void compute(double);
				static void sleep(double);
				bool isInitialized();
        bool isPlatformSetup();
        std::vector<std::string> getAllHostnames();
        void shutdown();
				static double getHostMemoryCapacity(std::string hostname);
				static double getMemoryCapacity();



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
