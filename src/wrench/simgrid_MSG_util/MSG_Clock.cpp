/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @file    Clock.cpp
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

#include "MSG_Clock.h"
#include <simgrid/msg.h>

namespace wrench {

		double MSG_Clock::getClock() {
			return MSG_get_clock();
		}

};