//
// Created by Henri Casanova on 3/9/17.
//

#include <iostream>
#include "S4U_Simulation.h"

namespace WRENCH {

		void S4U_Simulation::initialize(int *argc, char **argv) {
			this->engine = new simgrid::s4u::Engine(argc, argv);
		}

		void S4U_Simulation::runSimulation() {
			this->engine->run();
		}

		void S4U_Simulation::setupPlatform(std::string filename) {
			this->engine->loadPlatform(filename.c_str());
		}

		std::string S4U_Simulation::getHostName() {
			return simgrid::s4u::Host::current()->name();
		}

		// TODO: Implement
		void S4U_Simulation::compute(double flop) {
			// TODO: Should do an execute() there, but it doesn't work.... for now
			simgrid::s4u::this_actor::sleep_for(flop);
		}

		int S4U_Simulation::getNumCores(std::string hostname) {
			return simgrid::s4u::Host::by_name(hostname)->coreCount();
		}

		 double S4U_Simulation::getClock() {
			 return simgrid::s4u::Engine::instance()->getClock();
		 }


};