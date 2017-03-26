/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief wrench::SimpleWMSDaemon implements the daemon for a simple WMS abstraction
 */

#include <iostream>
#include <memory>

#include <exception/WRENCHException.h>

#include "simgrid_S4U_util/S4U_Mailbox.h"
#include "wms/engine/simple_wms/SimpleWMSDaemon.h"
#include "simulation/Simulation.h"
#include "workflow_job/StandardJob.h"
#include "workflow_job/PilotJob.h"
#include "job_manager/JobManager.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simple_wms_daemon, "Log category for Simple WMS Daemon");

namespace wrench {

	/**
	 * @brief Constructor
	 *
	 * @param simulation is a pointer to a Simulation object
	 * @param workflow is a pointer to a Workflow object
	 * @param scheduler is a pointer to a Scheduler object
	 */
	SimpleWMSDaemon::SimpleWMSDaemon(Simulation *simulation, Workflow *workflow, std::unique_ptr<Scheduler> scheduler) :
			EngineDaemon(simulation, workflow, std::move(scheduler)) {}

	/**
	 * @brief main method of the WMS daemon
	 *
	 * @return 0 on completion
	 */
	int SimpleWMSDaemon::main() {
		XBT_INFO("Starting on host %s listening on mailbox %s", S4U_Simulation::getHostName().c_str(),
		         this->mailbox_name.c_str());
		XBT_INFO("About to execute a workflow with %lu tasks", this->workflow->getNumberOfTasks());

		// Create a job manager
		std::unique_ptr<JobManager> job_manager = std::unique_ptr<JobManager>(new JobManager(this->workflow));

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
			this->scheduler->runTasks(job_manager.get(), ready_tasks, compute_services);

			// Wait for a workflow execution event
			std::unique_ptr<WorkflowExecutionEvent> event = workflow->waitForNextExecutionEvent();

			switch (event->type) {
				case WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
					StandardJob *job = (StandardJob *) (event->job);
					XBT_INFO("Notified that a %ld-task job has completed", job->getNumTasks());
					break;
				}
				case WorkflowExecutionEvent::STANDARD_JOB_FAILURE: {
					XBT_INFO("Notified that as job has failed (it's back in the ready state)");
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
