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


#include <workflow_job/WorkflowJob.h>
#include <workflow_job/StandardJob.h>
#include "workflow/WorkflowTask.h"

namespace wrench {

	class Simulation;

	class ComputeService {

	public:

		enum State {
			UP,
			DOWN
		};

		/** Constructors **/
		ComputeService(std::string, Simulation *simulation);
		ComputeService(std::string);

		/** Job execution **/
		virtual int runJob(StandardJob *job);

		/** Information getting **/
		virtual unsigned long numIdleCores() = 0;
		std::string getName();
		ComputeService::State getState();

		/** Stopping **/
		virtual void stop();


	protected:
		ComputeService::State state;
		std::string service_name;
		Simulation *simulation;  // pointer to the simulation object
	};
};


#endif //SIMULATION_COMPUTESERVICE_H
