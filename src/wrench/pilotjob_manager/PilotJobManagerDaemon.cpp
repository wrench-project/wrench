/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief TBD
 */

#include "PilotJobManagerDaemon.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(pilotjob_manager_daemon, "Log category for PilotJob Manager Daemon");

namespace wrench {

	PilotJobManagerDaemon::PilotJobManagerDaemon(): S4U_DaemonWithMailbox("pilotjob_manager", "pilotjob_manager") {

	}


	int PilotJobManagerDaemon::main() {
		XBT_INFO("New Multicore Task Executor starting (%s)", this->mailbox_name.c_str());


		XBT_INFO("New Multicore Task Executor terminating");
		return 0;
	}

};