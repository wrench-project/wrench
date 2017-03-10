/**
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

namespace WRENCH {

		double MSG_Clock::getClock() {
			return MSG_get_clock();
		}

};