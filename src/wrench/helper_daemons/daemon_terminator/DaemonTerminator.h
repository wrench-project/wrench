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

#ifndef WRENCH_DAEMONTERMINATOR_H
#define WRENCH_DAEMONTERMINATOR_H


#include <string>

namespace wrench {

		class DaemonTerminatorDaemon;

		class DaemonTerminator {
		public:
				DaemonTerminator(std::string, std::string, double);
		private:
				std::unique_ptr<DaemonTerminatorDaemon> daemon;
		};

};


#endif //WRENCH_DAEMONTERMINATOR_H
