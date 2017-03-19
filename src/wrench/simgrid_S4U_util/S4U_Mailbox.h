/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief S4U_Mailbox is a S4U wrapper
 */

#ifndef WRENCH_S4U_MAILBOX_H
#define WRENCH_S4U_MAILBOX_H


#include <simulation/SimulationMessage.h>

namespace wrench {
		class S4U_Mailbox {

		public:
				static std::string generateUniqueMailboxName(std::string);
				static std::unique_ptr<SimulationMessage> get(std::string mailbox);
				static void put(std::string mailbox, SimulationMessage *m);
		};
};


#endif //WRENCH_S4U_MAILBOX_H
