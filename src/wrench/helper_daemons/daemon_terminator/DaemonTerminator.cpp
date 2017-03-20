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

#include "DaemonTerminator.h"
#include "DaemonTerminatorDaemon.h"

namespace wrench {

		DaemonTerminator::DaemonTerminator(std::string hostname, std::string callback_mailbox, double time_to_death) {
			// Create the daemon
			this->daemon = std::unique_ptr<DaemonTerminatorDaemon>(
							new DaemonTerminatorDaemon(callback_mailbox, time_to_death));
			// Start the daemon on the host
			this->daemon->start(hostname);
		}

};