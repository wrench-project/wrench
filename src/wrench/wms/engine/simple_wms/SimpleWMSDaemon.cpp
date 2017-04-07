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
#include <logging/ColorLogging.h>

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

		ColorLogging::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_GREEN);

		WRENCH_INFO("Starting on host %s listening on mailbox %s", S4U_Simulation::getHostName().c_str(),
		         this->mailbox_name.c_str());
		WRENCH_INFO("About to execute a workflow with %lu tasks", this->workflow->getNumberOfTasks());

		// Create a job manager
		std::unique_ptr<JobManager> job_manager = std::unique_ptr<JobManager>(new JobManager(this->workflow));

		while (true) {

			// Take care of previously posted iput() that should be cleared
			S4U_Mailbox::clear_dputs();

			// Get the ready tasks
			std::vector<WorkflowTask *> ready_tasks = this->workflow->getReadyTasks();

			// Get the available compute services
			std::set<ComputeService *> compute_services = this->simulation->getComputeServices();
			if (compute_services.size() == 0) {
				WRENCH_INFO("Aborting - No compute services available!");
				break;
			}

			// Submit pilot jobs
			WRENCH_INFO("Scheduling pilot jobs...");
			double pilot_job_duration = 600.00; // bogus default
			if (ready_tasks.size() > 0) {
				// Heuristic: ask for something that can run  1.5 times the next ready tasks..
				pilot_job_duration = 1.5 * ready_tasks[0]->getFlops();
			}
			this->scheduler->schedulePilotJobs(job_manager.get(), this->workflow, pilot_job_duration, this->simulation->getComputeServices());

			// Run ready tasks with defined scheduler implementation
			WRENCH_INFO("Scheduling tasks...");
			this->scheduler->scheduleTasks(job_manager.get(), ready_tasks, this->simulation->getComputeServices());

			// Wait for a workflow execution event
			std::unique_ptr<WorkflowExecutionEvent> event = workflow->waitForNextExecutionEvent();

			switch (event->type) {
				case WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
					StandardJob *job = (StandardJob *) (event->job);
					WRENCH_INFO("Notified that a %ld-task job has completed", job->getNumTasks());
					break;
				}
				case WorkflowExecutionEvent::STANDARD_JOB_FAILURE: {
					WRENCH_INFO("Notified that a standard job has failed (it's back in the ready state)");
					break;
				}
				case WorkflowExecutionEvent::PILOT_JOB_START: {
					WRENCH_INFO("Notified that a pilot job has started!");
					break;
				}
				case WorkflowExecutionEvent::PILOT_JOB_EXPIRATION: {
					WRENCH_INFO("Notified that a pilot job has expired!");
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

		S4U_Mailbox::clear_dputs();

		if (workflow->isDone()) {
			WRENCH_INFO("Workflow execution is complete!");
		} else {
			WRENCH_INFO("Workflow execution is incomplete, but there are no more compute services...");
		}


		WRENCH_INFO("Simple WMS Daemon is shutting down all Compute Services");
		this->simulation->shutdownAllComputeServices();

		// This is brutal, but it's because that stupid job manager is currently
		// handling pilot job tersmination acks (due to the above shutdown), and
		// thus is stuck waiting for the WMS to receive them. But we're done. So,
		// for now, let's just kill it.
		// Perhaps this should be called in the destructor of the JobManager?
		// So that when the unique_ptr goes out of scope, the daemon dies...
		WRENCH_INFO("Killing the job manager");
		job_manager->kill();

		WRENCH_INFO("Simple WMS Daemon started on host %s terminating", S4U_Simulation::getHostName().c_str());

		return 0;
	}

};
