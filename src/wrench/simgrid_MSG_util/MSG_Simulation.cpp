/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
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

namespace wrench {

		void MSG_Simulation::initialize(int *argc, char **argv) {
			MSG_init(argc, argv);
		}

		void MSG_Simulation::runSimulation() {
			if (MSG_main() != MSG_OK) {
				throw WRENCHException("MSG_Simulation::runSimulation(): Error");
			}
		}

};