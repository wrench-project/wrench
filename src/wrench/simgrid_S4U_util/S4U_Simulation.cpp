/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include <iostream>
#include "S4U_Simulation.h"

namespace wrench {

		/*****************************/
		/**	INTERNAL METHODS BELOW **/
		/*****************************/

		/*! \cond INTERNAL */

		/**
		 * @brief Initialize the Simgrid simulation
		 *
		 * @param argc
		 * @param argv
		 */
		void S4U_Simulation::initialize(int *argc, char **argv) {
			this->engine = new simgrid::s4u::Engine(argc, argv);
		}

		/**
		 * @brief Start the simulation
		 */
		void S4U_Simulation::runSimulation() {
			this->engine->run();
		}

		/**
		 * @brief Initialize the simulated platform
		 *
		 * @param filename is the path to an XML platform file
		 */
		void S4U_Simulation::setupPlatform(std::string filename) {
			this->engine->loadPlatform(filename.c_str());
		}

		/**
		 * @brief Retrieves the hostname on which the calling actor is running
		 *
		 * @return the hostname
		 */
		std::string S4U_Simulation::getHostName() {
			return simgrid::s4u::Host::current()->name();
		}

		/**
		 * @brief Simulates a computation on host on which the calling actor is running
		 *
		 * @param flops is the number of flops
		 */
		void S4U_Simulation::compute(double flops) {
			simgrid::s4u::this_actor::execute(flops);
		}

		/**
		 * @brief Retrieves the number of cores of a host
		 *
		 * @param hostname is the name of the host
		 * @return the number of cores of the host
		 */
		int S4U_Simulation::getNumCores(std::string hostname) {
			return simgrid::s4u::Host::by_name(hostname)->coreCount();
		}

		/**
		 * @brief Retrieves the current simulation date
		 *
		 * @return the simulation clock
		 */
		double S4U_Simulation::getClock() {
			return simgrid::s4u::Engine::instance()->getClock();
		}

		/**
		 * @brief Simulates a sleep
		 * @param duration is the number of seconds to sleep
		 */
		 void S4U_Simulation::sleep(double duration) {
			return simgrid::s4u::this_actor::sleep_for(duration);
		}

		/*! \endcond  */

};