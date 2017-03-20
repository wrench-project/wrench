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


#ifndef WRENCH_DAEMONTERMINATORDAEMON_H
#define WRENCH_DAEMONTERMINATORDAEMON_H


#include <simgrid_S4U_util/S4U_DaemonWithMailbox.h>

namespace wrench {

		class DaemonTerminator;

		class DaemonTerminatorDaemon : public S4U_DaemonWithMailbox {

		public:
				DaemonTerminatorDaemon(std::string callback_mailbox, double time_to_death);

		private:
				int main();
				std::string callback_mailbox;
				double time_to_death;
		};
};


#endif //WRENCH_DAEMONTERMINATORDAEMON_H
