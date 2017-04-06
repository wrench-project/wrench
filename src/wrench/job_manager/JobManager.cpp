/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief JobManager implements a simple facility for managing
 *        jobs (standard and pilot) submitted by a WMS to compute services.
 */

#include <string>

#include "simgrid_S4U_util/S4U_Mailbox.h"
#include "exception/WRENCHException.h"
#include "simgrid_S4U_util/S4U_Simulation.h"
#include "JobManager.h"
#include "workflow_job/StandardJob.h"
#include "workflow_job/PilotJob.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(job_manager, "Log category for Job Manager");


namespace wrench {

		/**
		 * @brief Constructor, which starts the daemon
		 *
		 * @param workflow is a pointer to the Workflow whose jobs are to be managed
		 */
		JobManager::JobManager(Workflow *workflow) :
						S4U_DaemonWithMailbox("job_manager", "job_maanager") {

			this->workflow = workflow;

			// Start the daemon
			std::string localhost = S4U_Simulation::getHostName();
			this->start(localhost);
		}

		/**
		 * @brief Destructor
		 */
		JobManager::~JobManager() {
			this->stop();
			this->jobs.clear();
		}


		/**
		 * @brief Kill the job manager (brutally)
		 */
		void JobManager::kill() {
			this->kill_actor();
		}

		/**
		 * @brief Stop the job manager
		 */
		void JobManager::stop() {
				S4U_Mailbox::put(this->mailbox_name, new StopDaemonMessage());
		}

		/**
		 * @brief Create a standard job
		 *
		 * @param tasks is a vector of workflow task pointers to include in the job
		 * @return a raw pointer to the job
		 */
		StandardJob *JobManager::createStandardJob(std::vector<WorkflowTask *> tasks) {
			StandardJob *raw_ptr = new StandardJob(tasks);
			std::unique_ptr<WorkflowJob> job = std::unique_ptr<StandardJob>(raw_ptr);

			this->jobs[job->getName()] = std::move(job);
			return raw_ptr;
		}

		/**
		 * @brief Create a standard job
		 *
		 * @param task is a pointer to a workflow task
		 * @return a raw pointer to the job
		 */
		StandardJob *JobManager::createStandardJob(WorkflowTask * task) {
			std::vector<WorkflowTask *> tasks;
			tasks.push_back(task);
			return this->createStandardJob(tasks);
		}

		/**
		 * @brief Create a pilot job
		 *
		 * @param num_cores is the number of cores required by the job
		 * @param durations is the duration in seconds
		 * @return a raw pointer to the job
		 */
		PilotJob *JobManager::createPilotJob(Workflow *workflow, int num_cores, double duration) {
			PilotJob *raw_ptr = new PilotJob(workflow, num_cores, duration);
			std::unique_ptr<WorkflowJob> job = std::unique_ptr<PilotJob>(raw_ptr);
			this->jobs[job->getName()] = std::move(job);
			return raw_ptr;
		}

		/**
		 * @brief Submit a job to a compute service
		 *
		 * @param job is a pointer to a workflow job
		 * @param compute_service is a pointer to a compute service
		 */
		void JobManager::submitJob(WorkflowJob *job, ComputeService *compute_service) {

			// Check that this is valid submission
			if (!compute_service->canRunJob(job->getType(), job->getNumCores(), job->getDuration())) {
				throw WRENCHException("Compute service " + compute_service->getName() +
															" does not support " + job->getTypeAsString() + " jobs");
			}

			// Push back the mailbox of the manager,
			// so that it will get the initial callback
			job->pushCallbackMailbox(this->mailbox_name);

			// Update the job state and insert it into the pending list
			switch(job->getType()) {
				case WorkflowJob::STANDARD: {
					((StandardJob *)job)->state = StandardJob::PENDING;
					for (auto t : ((StandardJob *)job)->tasks) {
						t->setState(WorkflowTask::State::PENDING);
					}
					this->pending_standard_jobs.insert((StandardJob*)job);
					break;
				}
				case WorkflowJob::PILOT: {
					((PilotJob *)job)->state = PilotJob::PENDING;
					this->pending_pilot_jobs.insert((PilotJob*)job);
					break;
				}
			}

			// Submit the job to the service
			compute_service->runJob(job);

		}

		/**
		 * @brief Cancel a pilot job that hasn't expired yet
		 * @param job is the pilot job
		 */
		void JobManager::cancelPilotJob(PilotJob *job) {
			throw WRENCHException("cancelPilotJob() not implemented yet");
		}

		/**
		 * @brief Get the list of currently running pilot jobs
		 * @return a set of jobs
		 */
		std::set<PilotJob *> JobManager::getRunningPilotJobs() {
			return this->running_pilot_jobs;
		}

		/**
		 * @brief Get the list of currently pending pilot jobs
		 * @return  a set of jobs
		 */
		std::set<PilotJob *> JobManager::getPendingPilotJobs() {
			return this->pending_pilot_jobs;
		}

		/**
		 * @brief Forget a job (to free memory, typically once the job is completed)
		 *
		 */
		void JobManager::forgetJob(WorkflowJob *) {
			throw WRENCHException("forgetJob() not implemented yet");
		}


		/***********************************************************/
		/**	UNDOCUMENTED PUBLIC/PRIVATE  METHODS AFTER THIS POINT **/
		/***********************************************************/

		/*! \cond PRIVATE */

		/**
		 * @brief Main method of the job manager daemon
		 * @return 0 in success
		 */
		int JobManager::main() {
			XBT_INFO("New Job Manager starting (%s)", this->mailbox_name.c_str());

			bool keep_going = true;
			while (keep_going) {
				std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(this->mailbox_name);

				// Clear finished asynchronous dput()
				S4U_Mailbox::clear_dputs();

				XBT_INFO("Job Manager got a %s message", message->toString().c_str());
				switch (message->type) {

					case SimulationMessage::STOP_DAEMON: {
						// There should be any need to clean any state up
						keep_going = false;
						break;
					}

					case SimulationMessage::STANDARD_JOB_DONE: {
						std::unique_ptr<StandardJobDoneMessage> m(static_cast<StandardJobDoneMessage *>(message.release()));

						// update job state
						StandardJob *job = m->job;
						job->state = StandardJob::State::COMPLETED;

						// move the job from the "pending" list to the "completed" list
						this->pending_standard_jobs.erase(job);
						this->completed_standard_jobs.insert(job);

						// Forward the notification along the notification chain
						S4U_Mailbox::dput(job->popCallbackMailbox(),
															new StandardJobDoneMessage(job, m->compute_service));
						break;
					}

					case SimulationMessage::STANDARD_JOB_FAILED: {
						std::unique_ptr<StandardJobFailedMessage> m(static_cast<StandardJobFailedMessage *>(message.release()));

						// update job state
						StandardJob *job = m->job;
						job->state = StandardJob::State::FAILED;

						// remove the job from the "pending" list
						this->pending_standard_jobs.erase(job);

						// Forward the notification along the notification chain
						S4U_Mailbox::dput(job->popCallbackMailbox(),
															new StandardJobFailedMessage(job, m->compute_service));
						break;
					}

					case SimulationMessage::PILOT_JOB_STARTED: {
						std::unique_ptr<PilotJobStartedMessage> m(static_cast<PilotJobStartedMessage *>(message.release()));

						// update job state
						PilotJob *job = m->job;
						job->state = PilotJob::State::RUNNING;

						// move the job from the "pending" list to the "running" list
						this->pending_pilot_jobs.erase(job);
						this->running_pilot_jobs.insert(job);

						// Forward the notification to the source
						XBT_INFO("Forwarding to %s", job->getOriginCallbackMailbox().c_str());
						S4U_Mailbox::dput(job->getOriginCallbackMailbox(),
															new PilotJobStartedMessage(job, m->compute_service));

						break;
					}

					case SimulationMessage::PILOT_JOB_EXPIRED: {
						std::unique_ptr<PilotJobExpiredMessage> m(static_cast<PilotJobExpiredMessage *>(message.release()));

						// update job state
						PilotJob *job = m->job;
						job->state = PilotJob::State::EXPIRED;

						// Remove the job from the "running" list
						this->running_pilot_jobs.erase(job);
						XBT_INFO("THERE ARE NOW %ld running pilot jobs", this->running_pilot_jobs.size());

						// Forward the notification to the source
						XBT_INFO("Forwarding to %s", job->getOriginCallbackMailbox().c_str());
						S4U_Mailbox::dput(job->getOriginCallbackMailbox(),
															new PilotJobExpiredMessage(job, m->compute_service));

						break;
					}

					default: {
						throw WRENCHException("Invalid message type " + std::to_string(message->type));
					}
				}

			}

			XBT_INFO("New Multicore Task Executor terminating");
			return 0;
		}

		/*! \endcond */


};