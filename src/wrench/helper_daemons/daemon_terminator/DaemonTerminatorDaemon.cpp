/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief A simple daemon that sleeps for a particular duration and then stends
 *        a STOP_DAEMON message to some mailbox
 */


#include <simgrid_S4U_util/S4U_Mailbox.h>
#include "DaemonTerminatorDaemon.h"
#include "simgrid_S4U_util/S4U_Simulation.h"

namespace wrench {

		/**
		 * @brief Constructor
		 *
		 * @param callback_mailbox is the mailbox to which the "STOP_DAEMON" message will be sent
		 * @param time_to_death is the number of simulator seconds before the STOP_DAEMON message will be sent
		 */
		DaemonTerminatorDaemon::DaemonTerminatorDaemon(std::string callback_mailbox, double time_to_death) :
						S4U_DaemonWithMailbox("daemon_terminator", "none") {
			this->callback_mailbox = callback_mailbox;
			this->time_to_death = time_to_death;
		}

		/**
		 * @brief Main method of the daemon terminator daemon
		 *
		 * @return 0 on termination
		 */
		int DaemonTerminatorDaemon::main() {
			S4U_Simulation::sleep(this->time_to_death);
			S4U_Mailbox::put(this->callback_mailbox, new StopDaemonMessage());
			return 0;
		}

};