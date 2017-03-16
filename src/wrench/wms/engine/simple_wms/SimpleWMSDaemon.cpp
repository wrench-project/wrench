/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::SimpleWMSDaemon implements the daemon for a simple WMS abstraction
 */

#include <iostream>
#include <simgrid/msg.h>

#include "simgrid_S4U_util/S4U_Mailbox.h"
#include "wms/engine/simple_wms/SimpleWMSDaemon.h"
#include "simulation/Simulation.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simple_wms_daemon, "Log category for Simple WMS Daemon");

namespace wrench {

	/**
	 * @brief Constructor
	 *
	 * @param s is a pointer to the simulation
	 * @param w is a pointer to the workflow to execute
	 * @param sc is a pointer to a scheduler
	 */
	SimpleWMSDaemon::SimpleWMSDaemon(Simulation *s, Workflow *w, Scheduler *sc) :
			S4U_DaemonWithMailbox("simple_wms", "simple_wms") {
		this->simulation = s;
		this->workflow = w;
		this->scheduler = sc;
	}

	/**
	 * @brief main method of the WMS daemon
	 *
	 * @return 0 on completion
	 */
	int SimpleWMSDaemon::main() {
		XBT_INFO("Starting on host %s listening on mailbox %s", S4U_Simulation::getHostName().c_str(),
		         this->mailbox_name.c_str());
		XBT_INFO("About to execute a workflow with %lu tasks", this->workflow->getNumberOfTasks());

		while (true) {

			// Submit all ready tasks for execution
			std::vector<WorkflowTask *> ready_tasks = this->workflow->getReadyTasks();
			std::vector<std::unique_ptr<ComputeService>> &compute_services = this->simulation->getComputeServices();

			// Run ready tasks with defined scheduler implementation
			this->scheduler->runTasks(ready_tasks, compute_services);

			// Wait for a task completion
//			XBT_INFO("Waiting for a task to complete...");
			WorkflowTask *completed_task = workflow->waitForNextTaskCompletion();
			XBT_INFO("Notified that task %s has completed", completed_task->getId().c_str());

			if (workflow->isDone()) {
				break;
			}
		}

		XBT_INFO("Workflow execution is complete!");

		XBT_INFO("Simple WMS Daemon is shutting down all Compute Services");
		this->simulation->shutdown();

		XBT_INFO("Simple WMS Daemon started on host %s terminating", S4U_Simulation::getHostName().c_str());

		return 0;
	}

};
