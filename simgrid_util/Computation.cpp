/**
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


#include "Computation.h"

#include <simgrid/msg.h>


namespace WRENCH {

		void Computation::simulateComputation(double flops) {
			// Create a computational task
			msg_task_t t = MSG_task_create("", flops, 0.0, NULL);
			// Execute it on the local host
			MSG_task_execute(t);
			// Destroy it
			MSG_task_destroy(t);
			return;
		}


};