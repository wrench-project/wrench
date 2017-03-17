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
#include <exception/WRENCHException.h>

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

				// Get the ready tasks
				std::vector<WorkflowTask *> ready_tasks = this->workflow->getReadyTasks();

				// Get the available compute services
				std::set<ComputeService *> compute_services = this->simulation->getComputeServices();
				if (compute_services.size() == 0) {
					XBT_INFO("Aborting - No compute services available!");
					break;
				}

				// Run ready tasks with defined scheduler implementation
				this->scheduler->runTasks(ready_tasks, compute_services);

				// Wait for a workflow execution event
				std::unique_ptr<WorkflowExecutionEvent> event = workflow->wait_for_next_execution_event();

				switch(event->type) {
					case WorkflowExecutionEvent::TASK_COMPLETION: {
						XBT_INFO("Notified that task %s has completed", event->task->getId().c_str());
						break;
					}
					case WorkflowExecutionEvent::TASK_FAILURE: {
						XBT_INFO("Notified that task %s has failed (it's back in the ready state)", event->task->getId().c_str());
						break;
					}
					default: {
						throw WRENCHException("Unknown workflow execution event type");
					}
				}

				if (workflow->isDone()) {
					break;
				}
			}

			if (workflow->isDone()) {
				XBT_INFO("Workflow execution is complete!");
			} else {
				XBT_INFO("Workflow execution is incomplete, but there are no more compute services...");
			}

			XBT_INFO("Simple WMS Daemon is shutting down all Compute Services");
			this->simulation->shutdown();

			XBT_INFO("Simple WMS Daemon started on host %s terminating", S4U_Simulation::getHostName().c_str());

			return 0;
		}

};
