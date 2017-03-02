/**
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

namespace WRENCH {

		class Computation {

		public:
				static void simulateComputation(double second);
		};

}


#endif //SIMULATION_SIMGRIDCOMPUTATIONDELAY_H
