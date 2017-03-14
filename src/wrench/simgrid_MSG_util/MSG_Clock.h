/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
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


namespace wrench {


		class MSG_Clock {

		public:
				static double getClock();
		};

};


#endif //SIMULATION_CLOCK_H
