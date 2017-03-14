/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @file    Computation.h
 *  @author  Henri Casanova
 *  @date    2/24/2017
 *  @version 1.0
 *
 *  @brief WRENCH::Computation class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::Computation class is a MSG wrapper.
 *
 */


#ifndef SIMULATION_SIMGRIDCOMPUTATIONDELAY_H
#define SIMULATION_SIMGRIDCOMPUTATIONDELAY_H

namespace wrench {

		class MSG_Computation {

		public:
				static void simulateComputation(double second);
		};

}


#endif //SIMULATION_SIMGRIDCOMPUTATIONDELAY_H
