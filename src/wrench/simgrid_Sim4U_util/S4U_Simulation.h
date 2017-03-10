//
// Created by Henri Casanova on 3/9/17.
//

#ifndef WRENCH_S4U_SIMULATION_H
#define WRENCH_S4U_SIMULATION_H

#include <simgrid/s4u.hpp>

namespace WRENCH {

		class S4U_Simulation {

		public:
				void initialize(int *argc, char **argv);
				void setupPlatform(std::string);
				void runSimulation();

				static double getClock();

				static std::string getHostName();
				static int getNumCores(std::string hostname);

				static void compute(double);

		private:
				simgrid::s4u::Engine *engine;
		};
};


#endif //WRENCH_S4U_SIMULATION_H
