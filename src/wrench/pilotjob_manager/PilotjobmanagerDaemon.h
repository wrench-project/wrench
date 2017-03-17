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

#ifndef WRENCH_PILOGJOBMANAGERDAEMON_H
#define WRENCH_PILOGJOBMANAGERDAEMON_H


#include "simgrid_S4U_util/S4U_DaemonWithMailbox.h"

namespace wrench {

		class PilotjobManagerDaemon: public S4U_DaemonWithMailbox {

		public:
				PilotjobManagerDaemon();

		private:
				int main();

		};

};

#endif //WRENCH_PILOGJOBMANAGERDAEMON_H
