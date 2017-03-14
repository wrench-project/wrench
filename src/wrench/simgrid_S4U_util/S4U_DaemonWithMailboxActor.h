/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief S4U_DaemonWithMailboxDaemon implements the actor for a generic "running daemon that
 *        listens on a mailbox" abstraction
 */

#ifndef WRENCH_SIM4U_DAEMONWITHMAILBOXACTOR_H
#define WRENCH_SIM4U_DAEMONWITHMAILBOXACTOR_H

#include <xbt.h>
#include <string>
#include <vector>
#include <iostream>

#include "S4U_DaemonWithMailbox.h"

namespace wrench {

	class S4U_DaemonWithMailboxActor {

	public:

		explicit S4U_DaemonWithMailboxActor(S4U_DaemonWithMailbox *d) {
			this->daemon = d;
		}

		void operator()() {
			this->daemon->main();
		}

	private:
		S4U_DaemonWithMailbox *daemon;

	};
};


#endif //WRENCH_SIM4U_DAEMONWITHMAILBOXACTOR_H
