/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_SIMGRIDMAILBOX_H
#define WRENCH_SIMGRIDMAILBOX_H

#include <string>
#include "simulation/SimulationMessage.h"

namespace wrench {

		class MSG_Mailbox {
		public:
				static std::unique_ptr<SimulationMessage> get(std::string mailbox);
				static void put(std::string mailbox, SimulationMessage *m);
				static void iput(std::string mailbox, SimulationMessage *m);
				static void iput_failure_handler(void*);

		};

};


#endif //WRENCH_SIMGRIDMAILBOX_H
