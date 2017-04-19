/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_ENGINEDAEMON_H
#define WRENCH_ENGINEDAEMON_H

#include "simgrid_S4U_util/S4U_DaemonWithMailbox.h"
#include "workflow/Workflow.h"
#include "wms/scheduler/Scheduler.h"

namespace wrench {

		/***********************/
		/** \cond DEVELOPER    */
		/***********************/

		/**
		 * @brief Abstract implementation of WMS engine
		 */
	class EngineDaemon : public S4U_DaemonWithMailbox {

	public:
		/**
		 * @brief Constructor
		 *
		 * @param simulation is a pointer to the simulation
		 * @param workflow is a pointer to the workflow to execute
		 * @param scheduler is a pointer to a scheduler
		 */
		EngineDaemon(Simulation *simulation, Workflow *workflow, std::unique_ptr<Scheduler> scheduler)
				: simulation(simulation), workflow(workflow), scheduler(std::move(scheduler)),
				  S4U_DaemonWithMailbox("simple_wms", "simple_wms") {}

	protected:
		Simulation *simulation;
		Workflow *workflow;
		std::unique_ptr<Scheduler> scheduler;
	};

		/***********************/
		/** \cond DEVELOPER    */
		/***********************/

}

#endif //WRENCH_ENGINEDAEMON_H
