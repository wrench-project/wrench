/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief wrench::MulticoreStandardJobExecutorDaemon implements the daemon for the
 *  MulticoreStandardJobExecutor  Compute Service abstraction.
 */

#include "compute_services/multicore_job_executor/MulticoreStandardJobExecutorDaemon.h"
#include "exception/WRENCHException.h"
#include "simgrid_S4U_util/S4U_Mailbox.h"
#include "simgrid_S4U_util/S4U_Simulation.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(multicore_task_executor_daemon, "Log category for Multicore Standard Job Executor Daemon");

namespace wrench {

		/**
		 * @brief Constructor
		 *
		 * @param executors is a vector of sequential task executors
		 * @param cs is a pointer to the compute service for this daemon
		 */
		MulticoreStandardJobExecutorDaemon::MulticoreStandardJobExecutorDaemon(
						ComputeService *cs) : S4U_DaemonWithMailbox("multicore_standard_job_executor", "multicore_standard_job_executor") {



			// Set the pointer to the corresponding compute service
			this->compute_service = cs;
		}

		/**
		 * @brief Whether the executor has at least an idle core
		 *
		 * @return True when idle
		 */
		bool MulticoreStandardJobExecutorDaemon::hasIdleCore() {
			return this->idle_sequential_task_executors.size() > 0;
		}

		/**
		 * @brief Main method of the daemon
		 *
		 * @return 0 on termination
		 */
		int MulticoreStandardJobExecutorDaemon::main() {
			XBT_INFO("New Multicore Job Executor starting (%s) with %ld cores ",
							 this->mailbox_name.c_str(), this->idle_sequential_task_executors.size());

			// Start one sequential task executor daemon per core
			int num_cores = S4U_Simulation::getNumCores(S4U_Simulation::getHostName());

			for (int i = 0; i < num_cores; i++) {
				// Start a sequential task executor (unregistered to the simulation)
//			XBT_INFO("Starting a task executor on core #%d", i);
				std::unique_ptr<SequentialTaskExecutor> seq_executor =
								std::unique_ptr<SequentialTaskExecutor>(
												new SequentialTaskExecutor(S4U_Simulation::getHostName(), this->mailbox_name));
				// Add it to the vector of sequential task executors
				this->sequential_task_executors.push_back(std::move(seq_executor));
			}

			// Initialize the set of idle executors (cores)
			for (int i = 0; i < this->sequential_task_executors.size(); i++) {
				this->idle_sequential_task_executors.insert(this->sequential_task_executors[i].get());
			}

			/** Main loop **/
			bool keep_going = true;
			while (keep_going) {

				// Wait for a message
//				XBT_INFO("Waiting for a message");
				std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(this->mailbox_name);
				// Process the message
				switch (message->type) {


					case SimulationMessage::STOP_DAEMON: {

						XBT_INFO("Asked to terminate");

						// Kill all running sequential executors
						for (auto executor : this->busy_sequential_task_executors) {
							XBT_INFO("Killing a sequential task executor");
							executor->kill();
						}

						// Cleanly terminate all idle sequential executors
						for (auto executor : this->idle_sequential_task_executors) {
							executor->stop();
						}

						// Update all task states and send appropriate "job failed" messages
						for (auto failed_job : this->pending_jobs) {
							for (auto failed_task: failed_job->tasks) {
								failed_task->state = WorkflowTask::READY;
							}
							S4U_Mailbox::put(failed_job->pop_callback_mailbox(),
															 new JobFailedMessage(failed_job, this->compute_service));
						}

						// Break out of the mail loop
						keep_going = false;
						break;
					}

					case SimulationMessage::RUN_STANDARD_JOB: {
						std::unique_ptr<RunJobMessage> m(static_cast<RunJobMessage *>(message.release()));
						StandardJob *job = (StandardJob *)(m->job);
						XBT_INFO("Asked to run a job with %ld tasks", job->tasks.size());
						// Add all its tasks to the wait queue
						for (auto t : job->tasks) {
							this->waiting_task_queue.push(t);
						}

						break;
					}

					case SimulationMessage::TASK_DONE: {
						std::unique_ptr<TaskDoneMessage> m(static_cast<TaskDoneMessage *>(message.release()));

						StandardJob *job = (StandardJob *)(m->task->job);
						XBT_INFO("One of my cores completed task %s", m->task->id.c_str());

						// Remove the task from the running task queue
						this->running_task_set.erase(m->task);

						// Put that core's executor back into the pull of idle cores
						SequentialTaskExecutor *executor = m->task_executor;
						this->busy_sequential_task_executors.erase(executor);
						this->idle_sequential_task_executors.insert(executor);

						// Increase the "completed tasks" count of the job
						job->num_completed_tasks++;

						// Send the callback to the originator if necessary and remove the job from
						// the list of pending jobs
						if (job->num_completed_tasks == job->tasks.size()) {
							this->pending_jobs.erase(job);
							S4U_Mailbox::put(job->pop_callback_mailbox(),
															 new JobDoneMessage(job, this->compute_service));
						}
						break;
					}

					case SimulationMessage::NUM_IDLE_CORES_REQUEST: {
						std::unique_ptr<NumIdleCoresRequestMessage> m(static_cast<NumIdleCoresRequestMessage *>(message.release()));
						NumIdleCoresAnswerMessage *msg = new NumIdleCoresAnswerMessage(this->idle_sequential_task_executors.size());
						S4U_Mailbox::put(this->mailbox_name+"_answers", msg);
						break;
					}

					default: {
						XBT_INFO("MESSAGE TYPE = %d", message->type);
						throw WRENCHException("Unknown message type: " + std::to_string(message->type));
					}
				}

				// Run tasks while possible
				while ((waiting_task_queue.size() > 0) && (idle_sequential_task_executors.size() > 0)) {
					// Get the task to run the first task on the first idle core
					WorkflowTask *to_run = waiting_task_queue.front();
					waiting_task_queue.pop();

					// Get an idle sequential task executor and mark it as busy
					SequentialTaskExecutor *executor = *(this->idle_sequential_task_executors.begin());
					this->idle_sequential_task_executors.erase(executor);
					this->busy_sequential_task_executors.insert(executor);

					// Start the task on the sequential task executor
					XBT_INFO("Running task %s on one of my cores", to_run->id.c_str());
					executor->runTask(to_run);

					// Put the task in the running task set
					this->running_task_set.insert(to_run);
				}
			}

			XBT_INFO("Multicore Task Executor Daemon on host %s terminated!", S4U_Simulation::getHostName().c_str());
			return 0;
		}

};
