/**
 *  @file    Simgrid.h
 *  @author  Henri Casanova
 *  @date    2/25/2017
 *  @version 1.0
 *
 *  @brief WRENCH::Simgrid class implementations
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::Simgrid is a MSG wrapper
 *
 */


#ifndef SIMULATION_SIMGRID_H
#define SIMULATION_SIMGRID_H

namespace WRENCH {

		class MSG_Simulation {

		public:
				static void initialize(int *argc, char **argv);
				static void runSimulation();

		};

};


#endif //SIMULATION_SIMGRID_H
