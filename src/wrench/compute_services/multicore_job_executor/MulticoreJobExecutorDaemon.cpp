/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief wrench::MulticoreJobExecutorDaemon implements the daemon for the
 *  MulticoreStandardJobExecutor  Compute Service abstraction.
 */

#include <workflow_job/PilotJob.h>
#include <workflow_job/StandardJob.h>
#include <simulation/Simulation.h>
#include "compute_services/multicore_job_executor/MulticoreJobExecutorDaemon.h"
#include "exception/WRENCHException.h"
#include "simgrid_S4U_util/S4U_Mailbox.h"
#include "simgrid_S4U_util/S4U_Simulation.h"
#include "MulticoreJobExecutor.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(multicore_job_executor_daemon, "Log category for Multicore Job Executor Daemon");

namespace wrench {

		/**
		 * @brief Constructor
		 *
		 * @param cs is a pointer to the compute service for this daemon
		 * @param num_worker_threads is the number of worker threads (i.e., sequential task executors) - default value -1 means "use one thread per core"
		 * @param ttl is the time-to-live (in seconds) - default value -1.0 means "run forever"
		 */
		MulticoreJobExecutorDaemon::MulticoreJobExecutorDaemon(
						ComputeService *cs,
						int num_worker_threads,
						double ttl,
						PilotJob *pj,
						std::string suffix) : S4U_DaemonWithMailbox("multicore_job_executor" + suffix, "multicore_job_executor" + suffix) {

			this->compute_service = cs;
			this->num_worker_threads = num_worker_threads;
			this->ttl = ttl;
			this->has_ttl = (ttl > 0);
			this->containing_pilot_job = pj;
			this->num_available_worker_threads = this->num_worker_threads;
		}

		/**
		 * @brief Main method of the daemon
		 *
		 * @return 0 on termination
		 */
		int MulticoreJobExecutorDaemon::main() {

			/** Initialize all state **/
			initialize();

			double death_date = -1.0;
			if (this->has_ttl) {
				death_date = S4U_Simulation::getClock() + this->ttl;
			}

			XBT_INFO("DEATH DATE = %lf", death_date);

			if (this->containing_pilot_job) {
				XBT_INFO("MY CONTAINING PILOT JOB HAS CALLBACK MAILBOX '%s'", this->containing_pilot_job->getCallbackMailbox().c_str());
			}

			/** Main loop **/
			while (this->processNextMessage((this->has_ttl ? death_date - S4U_Simulation::getClock() : -1.0))) {

				// Clear pending asynchronous puts that are done
				S4U_Mailbox::clear_dputs();

				/** Dispatch currently pending tasks until no longer possible **/
				while (this->dispatchNextPendingTask());

				/** Dispatch jobs (and their tasks in the case of standard jobs) if possible) **/
				while (this->dispatchNextPendingJob());

			}

			XBT_INFO("Multicore Standard Job Executor Daemon on host %s terminated!", S4U_Simulation::getHostName().c_str());
			return 0;
		}

		/**
		 * @brief Dispatch one pending job, if possible
		 * @return true if a job was dispatched, false otherwise
		 */
		bool MulticoreJobExecutorDaemon::dispatchNextPendingJob() {

			/** While some idle core, then see if they can be used **/
			if ((this->busy_sequential_task_executors.size() < this->num_available_worker_threads)) {

				/** Look at pending jobs **/
				if (this->pending_jobs.size() > 0) {

					WorkflowJob *next_job = this->pending_jobs.front();

					switch (next_job->getType()) {
						case WorkflowJob::STANDARD: {
							StandardJob *job = (StandardJob *)next_job;

							// Put the job in the running queue
							this->pending_jobs.pop();
							this->running_jobs.insert(next_job);

							// Enqueue all its tasks in the task wait queue
							for (auto t : job->getTasks()) {
								this->pending_tasks.push(t);
							}

							// Try to dispatch its tasks if possible
							while (this->dispatchNextPendingTask());
							return true;
						}
						case WorkflowJob::PILOT: {
							PilotJob *job = (PilotJob *)next_job;
							XBT_INFO("Looking at dispatching pilot job %s with callbackmailbox %s",
											 job->getName().c_str(),
											 job->getCallbackMailbox().c_str());
							if (this->num_available_worker_threads - this->busy_sequential_task_executors.size() >
									job->getNumCores()) {

								// Create and launch a compute service
								ComputeService *cs =
												Simulation::createUnregisteredMulticoreJobExecutor(S4U_Simulation::getHostName(),
																																					 "yes", "no",
																																					 job->getNumCores(),
																																					 job->getDuration(),
																																					 job,
																																					 "_pilot");
								// Create and launch a compute service for the pilot job
								job->setComputeService(cs);

								// Reduce the number of available worker threads
								this->num_available_worker_threads -= job->getNumCores();

								// Put the job in the runnint queue
								this->pending_jobs.pop();
								this->running_jobs.insert(next_job);

								// Send the "Pilot job has started" callback
								// Note the getCallbackMailbox instead of the popCallbackMailbox, because
								// there will be another callback upon termination.
								S4U_Mailbox::dput(job->getCallbackMailbox(),
																 new PilotJobStartedMessage(job, this->compute_service));

								// Push my own mailbox onto the pilot job!
								job->pushCallbackMailbox(this->mailbox_name);
								return true;
							}
							break;
						}
					}
				}
			}
			return false;
		}

		/**
		 * @brief Dispatch one pending task to available worker threads (i.e., sequential task executors), if possible
		 * @return true if a task was dispatched, false otherwise
		 */
		bool MulticoreJobExecutorDaemon::dispatchNextPendingTask() {
			/** Dispatch tasks of currently running standard jobs to idle available worker threads **/
			if ((pending_tasks.size() > 0) &&
					(this->busy_sequential_task_executors.size() < this->num_available_worker_threads)) {

				// Get the first task out of the task wait queue
				WorkflowTask *to_run = pending_tasks.front();
				pending_tasks.pop();

				// Get the first idle sequential task executor and mark it as busy
				SequentialTaskExecutor *executor = *(this->idle_sequential_task_executors.begin());
				this->idle_sequential_task_executors.erase(executor);
				this->busy_sequential_task_executors.insert(executor);

				// Start the task on the sequential task executor
				XBT_INFO("Running task %s on one of my worker threads", to_run->getId().c_str());
				executor->runTask(to_run);

				// Put the task in the running task set
				this->running_tasks.insert(to_run);
				return true;

			} else {
				return false;
			}
		}


		/**
		 * @brief Wait for and react to any incoming message
		 * @return false if the daemon should terminate, true otherwise
		 */
		bool MulticoreJobExecutorDaemon::processNextMessage(double timeout) {

			// Wait for a message
			std::unique_ptr<SimulationMessage> message;

			if (this->has_ttl) {
				if (timeout <= 0) {
					return false;
				} else {
					XBT_INFO("Waiting for a message with timeout %lf", timeout);
					message = S4U_Mailbox::get(this->mailbox_name, timeout);
				}
			} else {
				message = S4U_Mailbox::get(this->mailbox_name);
			}

			// timeout
			if (message == nullptr) {
				XBT_INFO("TIMEOUT!!");
				this->terminate();
				return false;
			}

			XBT_INFO("Got a [%s] message", message->toString().c_str());

			switch (message->type) {

				case SimulationMessage::STOP_DAEMON: {
					this->terminate();
					S4U_Mailbox::put(this->mailbox_name + "_kill", new DaemonStoppedMessage());
					return false;
				}

				case SimulationMessage::RUN_STANDARD_JOB: {
					std::unique_ptr<RunStandardJobMessage> m(static_cast<RunStandardJobMessage *>(message.release()));
					XBT_INFO("Asked to run a standard job with %ld tasks", m->job->getNumTasks());
					this->pending_jobs.push(m->job);
					return true;
				}

				case SimulationMessage::RUN_PILOT_JOB: {
					std::unique_ptr<RunPilotJobMessage> m(static_cast<RunPilotJobMessage *>(message.release()));
					XBT_INFO("Asked to run a pilot job with %d cores for %lf seconds", m->job->getNumCores(), m->job->getDuration());
					this->pending_jobs.push(m->job);
					return true;
				}

				case SimulationMessage::TASK_DONE: {
					std::unique_ptr<TaskDoneMessage> m(static_cast<TaskDoneMessage *>(message.release()));
					processTaskCompletion(m->task, m->task_executor);
					return true;
				}

				case SimulationMessage::PILOT_JOB_EXPIRED: {
					std::unique_ptr<PilotJobExpiredMessage> m(static_cast<PilotJobExpiredMessage *>(message.release()));
					processPilotJobCompletion(m->job);
					return true;
				}

				case SimulationMessage::NUM_IDLE_CORES_REQUEST: {
					std::unique_ptr<NumIdleCoresRequestMessage> m(static_cast<NumIdleCoresRequestMessage *>(message.release()));
					NumIdleCoresAnswerMessage *msg = new NumIdleCoresAnswerMessage(this->idle_sequential_task_executors.size());
					S4U_Mailbox::dput(this->mailbox_name+"_answers", msg);
					return true;
				}

				case SimulationMessage::TTL_REQUEST: {
					std::unique_ptr<TTLRequestMessage> m(static_cast<TTLRequestMessage *>(message.release()));
					TTLAnswerMessage *msg = new TTLAnswerMessage(this->ttl);
					S4U_Mailbox::dput(this->mailbox_name+"_answers", msg);
					return true;
				}

				default: {
					throw WRENCHException("Unknown message type: " + std::to_string(message->type));
				}
			}
		}

		/**
		 * @brief Terminate all pilot job compute services
		 */
		void MulticoreJobExecutorDaemon::terminateAllPilotJobs() {
			for (auto job : this->running_jobs) {
				if (job->getType() == WorkflowJob::PILOT) {
					PilotJob *pj = (PilotJob *)job;
					pj->getComputeService()->stop();
				}
			}
		}


		/**
		 * @brief Terminate (nicely or brutally) all worker threads (i.e., sequential task executors)
		 */
		void MulticoreJobExecutorDaemon::terminateAllWorkerThreads() {
			// Kill all running sequential executors
			for (auto executor : this->busy_sequential_task_executors) {
				XBT_INFO("Brutally killing a busy sequential task executor");
				executor->kill();
			}

			// Cleanly terminate all idle sequential executors
			for (auto executor : this->idle_sequential_task_executors) {
				XBT_INFO("Cleanly stopping an idle sequential task executor");
				executor->stop();
			}
		}

		/**
		 * @brief Declare all current jobs as failed (liekly because the daemon is being terminated)
		 */
		void MulticoreJobExecutorDaemon::failCurrentStandardJobs() {

			XBT_INFO("There are %ld pending jobs", this->pending_jobs.size());
			while (!this->pending_jobs.empty()) {
				WorkflowJob *workflow_job = this->pending_jobs.front();
				XBT_INFO("Failing job %s", workflow_job->getName().c_str());
				this->pending_jobs.pop();
				if (workflow_job->getType() == WorkflowJob::STANDARD) {
					StandardJob *job = (StandardJob *)workflow_job;
					// Set all tasks back to the READY state
					for (auto failed_task: ((StandardJob *)job)->getTasks()) {
						failed_task->state = WorkflowTask::READY;
					}
					// Send back a job failed message
					S4U_Mailbox::dput(job->popCallbackMailbox(),
													 new StandardJobFailedMessage(job, this->compute_service));
				}
			}

			XBT_INFO("There are %ld running jobs", this->running_jobs.size());
			for (auto workflow_job : this->running_jobs) {
				XBT_INFO("Failing job %s", workflow_job->getName().c_str());
				if (workflow_job->getType() == WorkflowJob::STANDARD) {
					StandardJob *job = (StandardJob *)workflow_job;
					// Set all tasks back to the READY state
					for (auto failed_task: ((StandardJob *)job)->getTasks()) {
						failed_task->state = WorkflowTask::READY;
					}
					// Send back a job failed message
					S4U_Mailbox::dput(job->popCallbackMailbox(),
													 new StandardJobFailedMessage(job, this->compute_service));
				}
			}
		}

/**
 * @brief Initialize all state for the daemon
 */
		void MulticoreJobExecutorDaemon::initialize() {
			/** Start worker threads **/
			// Figure out the number of worker threads
			if (this->num_worker_threads == -1) {
				this->num_worker_threads = S4U_Simulation::getNumCores(S4U_Simulation::getHostName());
			}

			XBT_INFO("New Multicore Standard Job Executor starting (%s) with %d worker threads ",
							 this->mailbox_name.c_str(), this->num_worker_threads);

			for (int i = 0; i < this->num_worker_threads; i++) {
				// XBT_INFO("Starting a task executor on core #%d", i);
				std::unique_ptr<SequentialTaskExecutor> seq_executor =
								std::unique_ptr<SequentialTaskExecutor>(
												new SequentialTaskExecutor(S4U_Simulation::getHostName(), this->mailbox_name));
				this->sequential_task_executors.push_back(std::move(seq_executor));
			}


			// Initialize the set of idle executors (cores)
			for (int i = 0; i < this->sequential_task_executors.size(); i++) {
				this->idle_sequential_task_executors.insert(this->sequential_task_executors[i].get());
			}


		}

/**
 * @brief Process a task completion
 *
 * @param task is the WorkflowTask that has completed
 * @param executor is a pointer to the worker thread (sequential task executor) that has completed it
 */
		void MulticoreJobExecutorDaemon::processTaskCompletion(WorkflowTask *task, SequentialTaskExecutor *executor) {
			StandardJob *job = (StandardJob *)(task->job);
			XBT_INFO("One of my cores completed task %s", task->id.c_str());

			// Remove the task from the running task queue
			this->running_tasks.erase(task);

			// Put that core's executor back into the pull of idle cores
			this->busy_sequential_task_executors.erase(executor);
			this->idle_sequential_task_executors.insert(executor);

			// Increase the "completed tasks" count of the job
			job->num_completed_tasks++;

			// Send the callback to the originator if necessary and remove the job from
			// the list of pending jobs
			if (job->num_completed_tasks == job->getNumTasks()) {
				this->running_jobs.erase(job);
				S4U_Mailbox::dput(job->popCallbackMailbox(),
												 new StandardJobDoneMessage(job, this->compute_service));
			}
		}

/**
 * @brief Terminate the daemon, dealing with pending/running jobs
 */
		void MulticoreJobExecutorDaemon::terminate() {

			this->compute_service->setStateToDown();

			XBT_INFO("Terminate all worker threads");
			this->terminateAllWorkerThreads();

			XBT_INFO("Failing current standard jobs");
			this->failCurrentStandardJobs();

			XBT_INFO("terminate all pilot jobs");
			this->terminateAllPilotJobs();

			// Am I myself a pilot job?
			if (this->containing_pilot_job) {

				XBT_INFO("Letting the level above that the pilot job has ended on mailbox %s", this->containing_pilot_job->getCallbackMailbox().c_str());
				S4U_Mailbox::dput(this->containing_pilot_job->popCallbackMailbox(),
												 new PilotJobExpiredMessage(this->containing_pilot_job, this->compute_service));

			}
		}

		/**
		 * @brief Process a pilot job completion
		 *
		 * @param job
		 */
		void MulticoreJobExecutorDaemon::processPilotJobCompletion(PilotJob *job) {

			// Remove the job from the running list
			this->running_jobs.erase(job);

			// Update the number of available cores
			this->num_available_worker_threads += job->getNumCores();

			// Forward the notification
			S4U_Mailbox::dput(job->popCallbackMailbox(),
											 new PilotJobExpiredMessage(job, this->compute_service));

			return;
		}

};
