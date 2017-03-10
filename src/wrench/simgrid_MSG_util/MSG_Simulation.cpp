/**
 *  @file    Simgrid.cpp
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


#include "MSG_Simulation.h"
#include "exception/WRENCHException.h"

#include <simgrid/msg.h>

namespace WRENCH {

		void MSG_Simulation::initialize(int *argc, char **argv) {
			MSG_init(argc, argv);
		}

		void MSG_Simulation::runSimulation() {
			if (MSG_main() != MSG_OK) {
				throw WRENCHException("MSG_Simulation::runSimulation(): Error");
			}
		}

};