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

XBT_LOG_NEW_DEFAULT_CATEGORY(multicore_job_executor, "Log category for Multicore Job Executor");


namespace wrench {

		/**
		 * @brief Constructor that starts the daemon for the service on a host,
		 *        registering it with a WRENCH Simulation
		 *
		 * @param simulation is a pointer to a Simulation
		 * @param hostname is the name of the host
		 * @param num_worker_threads is the number of worker threads (i.e., sequential task executors)
		 * @param ttl
		 */
		MulticoreJobExecutor::MulticoreJobExecutor(Simulation *simulation,
																							 std::string hostname,
																							 int num_worker_threads,
																							 double ttl,
																							 PilotJob *pj,
		                                           std::string suffix) :
						ComputeService("multicore_job_executor", simulation) {

			// Set all relevant properties
			this->setProperty(ComputeService::SUPPORTS_STANDARD_JOBS, "yes");
			this->setProperty(ComputeService::SUPPORTS_PILOT_JOBS, "no");

			// Create the main daemon
			this->daemon = std::unique_ptr<MulticoreJobExecutorDaemon>(
							new MulticoreJobExecutorDaemon(this, num_worker_threads, ttl, pj, suffix));

			// Start the daemon on the same host
			this->daemon->start(hostname);
		}


		/**
		 * @brief Stop the service
		 */
		void MulticoreJobExecutor::stop() {

			this->state = ComputeService::DOWN;

			if (this->daemon != nullptr) {
				XBT_INFO("Telling daemon listening on (%s) to terminate", this->daemon->mailbox_name.c_str());
				// Send a termination message to the daemon's mailbox
				S4U_Mailbox::put(this->daemon->mailbox_name, new StopDaemonMessage());
				// Wait for the ack
				std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(this->daemon->mailbox_name + "_kill");
				if (message->type != SimulationMessage::Type::DAEMON_STOPPED) {
					throw WRENCHException("Wrong message type received while expecting DAEMON_STOPPED");
				}
				this->daemon = nullptr;
			}
		}


		/**
		 * @brief Have the service execute a standard job
		 *
		 * @param job is a pointer a standard job
		 * @param callback_mailbox is the name of a mailbox to which a "job done" callback will be sent
		 */
		void MulticoreJobExecutor::runStandardJob(StandardJob *job) {

			if (this->getProperty(ComputeService::SUPPORTS_STANDARD_JOBS) != "yes") {
				throw WRENCHException("Implementation error: this compute service should have the SUPPORTS_STANDARD_JOBS property set to 'yes'");
			}

			if (this->state == ComputeService::DOWN) {
				throw WRENCHException("Trying to run a job on a compute service that's terminated");
			}

			// Synchronously send a "run a task" message to the daemon's mailbox
			S4U_Mailbox::dput(this->daemon->mailbox_name, new RunStandardJobMessage(job));
		};

		/**
		 * @brief Have the service execute a pilot job
		 *
		 * @param task is a pointer the pilot job
		 * @param callback_mailbox is the name of a mailbox to which a "pilot job started" callback will be sent
		 */
		void MulticoreJobExecutor::runPilotJob(PilotJob *job) {

			if (this->getProperty(ComputeService::SUPPORTS_PILOT_JOBS) != "yes") {
				throw WRENCHException("Implementation error: this compute service should have the SUPPORTS_PILOT_JOBS property set to 'yes'");
			}

			if (this->state == ComputeService::DOWN) {
				throw WRENCHException("Trying to run a job on a compute service that's terminated");
			}

			//  send a "run a task" message to the daemon's mailbox
			S4U_Mailbox::dput(this->daemon->mailbox_name, new RunPilotJobMessage(job));
		};


		/**
		 * @brief Finds out how many idle cores the  service has
		 *
		 * @return the number of currently idle cores
		 */
		unsigned long MulticoreJobExecutor::getNumIdleCores() {

			if (this->state == ComputeService::DOWN) {
				throw WRENCHException("Compute Service is down");
			}

			S4U_Mailbox::dput(this->daemon->mailbox_name, new NumIdleCoresRequestMessage());
			std::unique_ptr<SimulationMessage> msg= S4U_Mailbox::get(this->daemon->mailbox_name + "_answers");
			std::unique_ptr<NumIdleCoresAnswerMessage> m(static_cast<NumIdleCoresAnswerMessage *>(msg.release()));
			return m->num_idle_cores;
		}

		/**
		 * @brief Finds out the TTL
		 *
		 * @return the TTL in seconds
		 */
		double MulticoreJobExecutor::getTTL() {

			if (this->state == ComputeService::DOWN) {
				throw WRENCHException("Compute Service is down");
			}

			S4U_Mailbox::dput(this->daemon->mailbox_name, new TTLRequestMessage());
			std::unique_ptr<SimulationMessage> msg= S4U_Mailbox::get(this->daemon->mailbox_name + "_answers");
			std::unique_ptr<TTLAnswerMessage> m(static_cast<TTLAnswerMessage *>(msg.release()));
			return m->ttl;
		}

}