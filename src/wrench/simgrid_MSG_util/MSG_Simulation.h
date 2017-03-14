/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef SIMULATION_SIMGRID_H
#define SIMULATION_SIMGRID_H

namespace wrench {

		class MSG_Simulation {

		public:
				static void initialize(int *argc, char **argv);
				static void runSimulation();

		};

};


#endif //SIMULATION_SIMGRID_H
