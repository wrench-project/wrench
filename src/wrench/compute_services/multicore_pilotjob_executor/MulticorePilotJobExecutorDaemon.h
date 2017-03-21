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


#ifndef WRENCH_MULTICOREPILOTJOBEXECUTORDAEMON_H
#define WRENCH_MULTICOREPILOTJOBEXECUTORDAEMON_H


#include <simgrid_S4U_util/S4U_DaemonWithMailbox.h>

namespace wrench {

		class MulticorePilotJobExecutorDaemon : public S4U_DaemonWithMailbox {

		public:
				MulticorePilotJobExecutorDaemon();

		private:

				int main();

		};

};


#endif //WRENCH_MULTICOREPILOTJOBEXECUTORDAEMON_H
