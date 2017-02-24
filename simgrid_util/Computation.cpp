//
// Created by Henri Casanova on 2/23/17.
//

#include "Computation.h"

#include <simgrid/msg.h>



namespace WRENCH {

		void Computation::simulateComputation(double seconds) {
			msg_task_t t = MSG_task_create("",seconds, 0.0, NULL);
			MSG_task_execute(t);

			MSG_task_destroy(t);
			return;
		}


};