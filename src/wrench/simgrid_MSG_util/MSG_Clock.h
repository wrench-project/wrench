/**
 *  @file    Clock.h
 *  @author  Henri Casanova
 *  @date    2/24/2017
 *  @version 1.0
 *
 *  @brief WRENCH::Clock class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::Clock class is a MSG wrapper.
 *
 */


#ifndef SIMULATION_CLOCK_H
#define SIMULATION_CLOCK_H


namespace WRENCH {


		class MSG_Clock {

		public:
				static double getClock();
		};

};


#endif //SIMULATION_CLOCK_H
