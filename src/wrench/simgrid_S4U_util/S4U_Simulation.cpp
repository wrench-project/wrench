/**
 * @brief S4U_Simulation is an S4U Wrapper
 */

#include <iostream>
#include "S4U_Simulation.h"

namespace WRENCH {

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
			// TODO: Should do an execute() there, but it doesn't work.... for now
			simgrid::s4u::this_actor::sleep_for(flops);
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

};