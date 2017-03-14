/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::SequentialRandomWMSDaemon implements the daemon for a simple WMS abstraction
 */

#include <iostream>
#include <simgrid/msg.h>
#include <simgrid_S4U_util/S4U_Mailbox.h>
#include <exception/WRENCHException.h>

#include "SequentialRandomWMSDaemon.h"
#include "simulation/Simulation.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simple_wms_daemon, "Log category for Simple WMS Daemon");

namespace wrench {

	/**
	 * @brief Constructor
	 *
	 * @param s is a pointer to the simulation
	 * @param w is a pointer to the workflow to execute
	 */
	SequentialRandomWMSDaemon::SequentialRandomWMSDaemon(Simulation *s, Workflow *w) :
			S4U_DaemonWithMailbox("simple_wms", "simple_wms") {
		this->simulation = s;
		this->workflow = w;
	}

	/**
	 * @brief main method of the WMS daemon
	 *
	 * @return 0 on completion
	 */
	int SequentialRandomWMSDaemon::main() {
		XBT_INFO("Starting on host %s listening on mailbox %s", S4U_Simulation::getHostName().c_str(),
		         this->mailbox_name.c_str());
		XBT_INFO("About to execute a workflow with %lu tasks", this->workflow->getNumberOfTasks());

		/** Stupid Scheduling/Execution Algorithm **/

		while (true) {

			// Submit all ready tasks for execution
			std::vector<WorkflowTask *> ready_tasks = this->workflow->getReadyTasks();
			if (ready_tasks.size() > 0) {
				XBT_INFO("There are %ld ready tasks", ready_tasks.size());
			}
			for (int i = 0; i < ready_tasks.size(); i++) {
				XBT_INFO("Submitting task %s for execution", ready_tasks[i]->id.c_str());
				ready_tasks[i]->setScheduled();
				ComputeService *cs;

				cs = this->simulation->getSomeMulticoreTaskExecutor();
				if (!cs) {
					cs = this->simulation->getSomeSequentialTaskExecutor();
				}
				if (cs) {
					cs->runTask(ready_tasks[i], this->mailbox_name);
				} else {
					throw WRENCHException("No compute resources!");
				}
			}

			// Wait for a task completion
			XBT_INFO("Waiting for a task to complete...");
			std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(this->mailbox_name);
			std::unique_ptr<TaskDoneMessage> m(static_cast<TaskDoneMessage *>(message.release()));

			XBT_INFO("Notified that task %s has completed", m->task->id.c_str());

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
