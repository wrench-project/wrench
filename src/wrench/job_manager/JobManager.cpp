/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief PilotJobManager implements a simple facility for managing pilot
 *        jobs submitted by a WMS to pilot-job-enabled compute services.
 */

#include <string>

#include "simgrid_S4U_util/S4U_Mailbox.h"
#include "exception/WRENCHException.h"
#include "simgrid_S4U_util/S4U_Simulation.h"
#include "JobManager.h"
#include "JobManagerDaemon.h"
#include "workflow_job/StandardJob.h"
#include "workflow_job/PilotJob.h"

namespace wrench {

		/**
		 * @brief Constructor, which starts the daemon
		 *
		 * @param workflow is a pointer to the Workflow whose jobs are to be managed
		 */
		JobManager::JobManager(Workflow *workflow) {

			this->workflow = workflow;

			// Create the  daemon
			this->daemon = std::unique_ptr<JobManagerDaemon>(
							new JobManagerDaemon(this));

			// Start the daemon
			std::string localhost = S4U_Simulation::getHostName();
			this->daemon->start(localhost);
		}

		/**
		 * @brief Destructor
		 */
		JobManager::~JobManager() {
			this->stop();
			this->jobs.clear();
		}


		/**
		 * @brig Kill the job manager (brutally)
		 */
		void JobManager::kill() {
			this->daemon->kill_actor();
		}

		/**
		 * @brief Stop the job manager
		 */
		void JobManager::stop() {
			if (this->daemon != nullptr) {
				S4U_Mailbox::put(this->daemon->mailbox_name, new StopDaemonMessage());
				this->daemon = nullptr;
			}

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
			if (!compute_service->canRunJob(job)) {
				throw WRENCHException("Compute service " + compute_service->getName() +
															" does not support " + job->getTypeAsString() + " jobs");
			}

			// Push back the mailbox of the manager,
			// so that it will get the initial callback
			job->pushCallbackMailbox(this->daemon->mailbox_name);

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

		void JobManager::cancelPilotJob(PilotJob *job) {
			throw WRENCHException("cancelPilotJob() not implemented yet");
		}

		std::set<PilotJob *> JobManager::getRunningPilotJobs() {
			return this->running_pilot_jobs;
		}

		std::set<PilotJob *> JobManager::getPendingPilotJobs() {
			return this->pending_pilot_jobs;
		}

		void JobManager::forgetJob(WorkflowJob *) {
			throw WRENCHException("forgetJob() not implemented yet");
		}


};