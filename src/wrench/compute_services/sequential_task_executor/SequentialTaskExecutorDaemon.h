/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::SequentialTaskExecutorDaemon implements the daemon for the
 *  SequentialTaskExecutor Compute Service abstraction.
 */

#ifndef SIMULATION_SEQUENTIALTASKEXECUTORDAEMON_H
#define SIMULATION_SEQUENTIALTASKEXECUTORDAEMON_H

#include "compute_services/ComputeService.h"
#include "simgrid_S4U_util/S4U_DaemonWithMailbox.h"

namespace wrench {

	class SequentialTaskExecutorDaemon : public S4U_DaemonWithMailbox {

	public:
		SequentialTaskExecutorDaemon(ComputeService *cs);
		bool isIdle();

	private:
		int main();
		ComputeService *compute_service;
		bool busy;
	};
}

#endif //SIMULATION_SEQUENTIALTASKEXECUTORDAEMON_H
