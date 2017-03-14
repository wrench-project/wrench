/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @file    Computation.cpp
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


#include "MSG_Computation.h"
#include "exception/WRENCHException.h"
#include "MSG_Host.h"

#include <simgrid/msg.h>


namespace wrench {

		void MSG_Computation::simulateComputation(double flops) {
			// Create a computational task
			msg_task_t t = MSG_task_create("", flops, 0.0, NULL);
			// Execute it on the local host
			if (MSG_task_execute(t) != MSG_OK) {
				throw WRENCHException("Computation::simulateComputation(): Cannot execute task on host " + MSG_Host::getHostName());
			}
			// Destroy it
			if (MSG_task_destroy(t) != MSG_OK) {
				throw WRENCHException("Computation::simulateComputation(): Cannot destroy task");
			}
			return;
		}


};