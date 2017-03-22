/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief wrench::MulticoreJobExecutor implements a simple
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
	 * @param num_worker_threads is the number of worker threads (i.e., sequential task executors)
	 * @param simulation is a pointer to a Simulation
	 */
		MulticoreJobExecutor::MulticoreJobExecutor(Simulation *simulation,
																							 std::string hostname,
																							 int num_worker_threads,
																							 double ttl
		) :
						ComputeService("multicore_job_executor", simulation) {

			// Set all relevant properties
			this->setProperty(ComputeService::SUPPORTS_STANDARD_JOBS, "yes");
			this->setProperty(ComputeService::SUPPORTS_PILOT_JOBS, "no");

			// Create the main daemon
			this->daemon = std::unique_ptr<MulticoreJobExecutorDaemon>(
							new MulticoreJobExecutorDaemon(this, num_worker_threads, ttl));

			// Start the daemon on the same host
			this->daemon->start(hostname);

		}

		/**
		 * @brief Stop the service
		 */
		void MulticoreJobExecutor::stop() {
			// Send a termination message to the daemon's mailbox
			S4U_Mailbox::put(this->daemon->mailbox_name, new StopDaemonMessage());

			// Call the generic stopping method, which does some cleanup
			// and bookkeeping
			ComputeService::stop();
		}

		/**
		 * @brief Have the service execute a standard job
		 *
		 * @param job is a pointer a standard job
		 * @param callback_mailbox is the name of a mailbox to which a "job done" callback will be sent
		 * @return 0 on success
		 */
		int MulticoreJobExecutor::runStandardJob(StandardJob *job) {

			if (this->getProperty(ComputeService::SUPPORTS_STANDARD_JOBS) != "yes") {
				throw WRENCHException("Implementation error: this compute service should have the SUPPORTS_STANDARD_JOBS property set to 'yes'");
			}

			if (this->state == ComputeService::DOWN) {
				throw WRENCHException("Trying to run a job on a compute service that's terminated");
			}

			// Synchronously send a "run a task" message to the daemon's mailbox
			S4U_Mailbox::put(this->daemon->mailbox_name, new RunStandardJobMessage(job));
			return 0;
		};

		/**
	 * @brief Have the service execute a pilot job
	 *
	 * @param task is a pointer the pilot job
	 * @param callback_mailbox is the name of a mailbox to which a "pilot job started" callback will be sent
	 * @return 0 on success
	 */
		int MulticoreJobExecutor::runPilotJob(PilotJob *job) {

			if (this->getProperty(ComputeService::SUPPORTS_PILOT_JOBS) != "yes") {
				throw WRENCHException("Implementation error: this compute service should have the SUPPORTS_PILOT_JOBS property set to 'yes'");
			}

			if (this->state == ComputeService::DOWN) {
				throw WRENCHException("Trying to run a job on a compute service that's terminated");
			}

			// Synchronously send a "run a task" message to the daemon's mailbox
			S4U_Mailbox::put(this->daemon->mailbox_name, new RunPilotJobMessage(job));
			return 0;
		};


		/**
		 * @brief Finds out how many idle cores the  service has
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