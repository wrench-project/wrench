/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::ComputeService implements an abstract compute service.
 */


#ifndef SIMULATION_COMPUTESERVICE_H
#define SIMULATION_COMPUTESERVICE_H


#include "workflow/WorkflowTask.h"

namespace wrench {

	class Simulation;

	class ComputeService {

	public:

			enum State {
					RUNNING,
					TERMINATED
			};

			ComputeService(std::string, Simulation *simulation);

		// Virtual methods to implement in derived classes
		virtual int runTask(WorkflowTask *task) = 0;
		virtual bool hasIdleCore() = 0;
		virtual void stop();

		std::string getName();
		ComputeService::State getState();

	protected:
		ComputeService::State state;
		std::string service_name;
		Simulation *simulation;  // pointer to the simulation object
	};
};


#endif //SIMULATION_COMPUTESERVICE_H
