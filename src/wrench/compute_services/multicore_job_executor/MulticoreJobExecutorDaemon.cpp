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

#include <helper_daemons/daemon_terminator/DaemonTerminator.h>
#include "compute_services/multicore_job_executor/MulticoreJobExecutorDaemon.h"
#include "exception/WRENCHException.h"
#include "simgrid_S4U_util/S4U_Mailbox.h"
#include "simgrid_S4U_util/S4U_Simulation.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(multicore_standard_job_executor, "Log category for Multicore Standard Job Executor");

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
						double ttl) : S4U_DaemonWithMailbox("multicore_job_executor", "multicore_job_executor") {

			this->compute_service = cs;
			this->num_worker_threads = num_worker_threads;
			this->ttl = ttl;
		}

		/**
		 * @brief Main method of the daemon
		 *
		 * @return 0 on termination
		 */
		int MulticoreJobExecutorDaemon::main() {

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


			/** Start the terminator if needed **/
			if (this->ttl  > 0) {
				DaemonTerminator terminator(S4U_Simulation::getHostName(), this->mailbox_name, this->ttl);
			}


			/** Main loop **/
			bool keep_going = true;
			while (keep_going) {

				// Wait for a message
				std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(this->mailbox_name);
				switch (message->type) {

					case SimulationMessage::STOP_DAEMON: {
						XBT_INFO("Asked to terminate");
						this->terminate_all_worker_threads();
						this->fail_all_current_jobs();
						keep_going = false;
						break;
					}

					case SimulationMessage::RUN_STANDARD_JOB: {
						std::unique_ptr<RunStandardJobMessage> m(static_cast<RunStandardJobMessage *>(message.release()));
						StandardJob *job = (StandardJob *)(m->job);
						XBT_INFO("Asked to run a job with %ld tasks", job->tasks.size());
						// Add all its tasks to the wait queue
						for (auto t : job->tasks) {
							this->waiting_task_queue.push(t);
						}
						break;
					}

					case SimulationMessage::RUN_PILOT_JOB: {
						std::unique_ptr<RunPilotJobMessage> m(static_cast<RunPilotJobMessage *>(message.release()));
						PilotJob *job = (PilotJob *)(m->job);
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
															 new StandardJobDoneMessage(job, this->compute_service));
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

			XBT_INFO("Multicore Standard Job Executor Daemon on host %s terminated!", S4U_Simulation::getHostName().c_str());
			return 0;
		}


		void MulticoreJobExecutorDaemon::terminate_all_worker_threads() {
			// Kill all running sequential executors
			for (auto executor : this->busy_sequential_task_executors) {
				XBT_INFO("Killing a sequential task executor");
				executor->kill();
			}

			// Cleanly terminate all idle sequential executors
			for (auto executor : this->idle_sequential_task_executors) {
				executor->stop();
			}
		}

		void MulticoreJobExecutorDaemon::fail_all_current_jobs() {

			/* STANDARD JOBS */
			// Update all task states and send appropriate "job failed" messages
			for (auto failed_job : this->pending_jobs) {
				for (auto failed_task: failed_job->tasks) {
					failed_task->state = WorkflowTask::READY;
				}
				S4U_Mailbox::put(failed_job->pop_callback_mailbox(),
												 new StandardJobFailedMessage(failed_job, this->compute_service));
			}

			/* PILOT JOBS */
			// TODO
		}
};
