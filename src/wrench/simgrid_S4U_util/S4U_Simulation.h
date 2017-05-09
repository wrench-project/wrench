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
				void setupPlatform(std::string);
				void runSimulation();
				static double getClock();
				static std::string getHostName();
				static int getNumCores(std::string hostname);
				static void compute(double);
				static void sleep(double);
				bool isInitialized();
        std::vector<std::string> getAllHostnames();
        void shutdown();

		private:
				simgrid::s4u::Engine *engine;
				bool initialized = false;
		};

		/***********************/
		/** \endcond           */
		/***********************/
};

#endif //WRENCH_S4U_SIMULATION_H
