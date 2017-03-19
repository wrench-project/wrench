/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::MulticoreTaskExecutor implements a simple
 *  Compute Service abstraction for a multi-core task executor.
 *
 */

#include "workflow/WorkflowTask.h"
#include "simgrid_S4U_util/S4U_Mailbox.h"
#include "simgrid_S4U_util/S4U_Simulation.h"
#include "exception/WRENCHException.h"
#include "compute_services/multicore_job_executor/MulticoreJobExecutor.h"

namespace wrench {

	/**
	 * @brief Constructor that starts the daemon for the service on a host,
	 *        registering it with a WRENCH Simulation
	 *
	 * @param hostname is the name of the host
	 * @param simulation is a pointer to a Simulation
	 */
	MulticoreJobExecutor::MulticoreJobExecutor(std::string hostname, Simulation *simulation) :
					ComputeService("multicore_job_executor", simulation) {

		this->hostname = hostname;
		// Create the main daemon
		this->daemon = std::unique_ptr<MulticoreJobExecutorDaemon>(
						new MulticoreJobExecutorDaemon(this));

		// Start the daemon
		this->daemon->start(this->hostname);

	}

	/**
	 * @brief Stop the multi-core task executor
	 */
	void MulticoreJobExecutor::stop() {
		// Send a termination message to the daemon's mailbox
		S4U_Mailbox::put(this->daemon->mailbox_name, new StopDaemonMessage());

		// Call the generic stopping method
		ComputeService::stop();
	}

	/**
	 * @brief Have the service execute a workflow task
	 *
	 * @param task is a pointer the workflow task
	 * @param callback_mailbox is the name of a mailbox to which a "task done" callback will be sent
	 * @return 0 on success
	 */
	int MulticoreJobExecutor::runJob(StandardJob *job) {

		if (this->state == ComputeService::DOWN) {
			throw WRENCHException("Trying to run a task on a compute service that's terminated");
		}
		// Synchronously send a "run a task" message to the daemon's mailbox
		S4U_Mailbox::put(this->daemon->mailbox_name, new RunJobMessage(job));
		return 0;
	};

	/**
	 * @brief Finds out how many idle cores the compute service has
	 *
	 * @return
	 */
	unsigned long MulticoreJobExecutor::numIdleCores() {
		S4U_Mailbox::put(this->daemon->mailbox_name, new NumIdleCoresRequestMessage());
		std::unique_ptr<SimulationMessage> msg= S4U_Mailbox::get(this->daemon->mailbox_name + "_answers");
		std::unique_ptr<NumIdleCoresAnswerMessage> m(static_cast<NumIdleCoresAnswerMessage *>(msg.release()));
		return m->num_idle_cores;
	}
}