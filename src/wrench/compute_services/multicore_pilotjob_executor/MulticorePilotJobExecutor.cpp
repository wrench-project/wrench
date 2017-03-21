/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief wrench::MulticorePilotJobExecutor implements a simple
 *  Compute Service abstraction for a multi-core pilotjob executor.
 *
 */


#include "MulticorePilotJobExecutor.h"

namespace wrench {

		/**
	 * @brief Constructor that starts the daemon for the service on a host,
	 *        registering it with a WRENCH Simulation
	 *
	 * @param hostname is the name of the host
	 * @param simulation is a pointer to a Simulation
	 */
		MulticorePilotJobExecutor::MulticorePilotJobExecutor(std::string hostname, Simulation *simulation) :
						ComputeService("multicore_pilot_job_executor", simulation) {

			// Set all relevant properties
			this->setProperty(ComputeService::SUPPORTS_STANDARD_JOBS, "yes");
			this->setProperty(ComputeService::SUPPORTS_PILOT_JOBS, "no");

			// Create the main daemon
			this->daemon = std::unique_ptr<MulticoreJobExecutorDaemon>(
							new MulticoreJobExecutorDaemon(this));

			// Start the daemon on the same host
			this->daemon->start(hostname);

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
		int MulticoreJobExecutor::runStandardJob(StandardJob *job) {

			if (this->getProperty(ComputeService::SUPPORTS_STANDARD_JOBS) != "yes") {
				throw WRENCHException("Implementation error: this compute service should have the SUPPORTS_STANDARD_JOBS property set to 'yes'");
			}

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

};
