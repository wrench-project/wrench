/**
 *  @file    MulticoreTaskExecutorDaemon.cpp
 *  @author  Henri Casanova
 *  @date    3/7/2017
 *  @version 1.0
 *
 *  @brief WRENCH::MulticoreTaskExecutorDaemon class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::MulticoreTaskExecutorDaemon class implements the daemon for a simple
 *  Compute Service abstraction.
 *
 */

#include "MulticoreTaskExecutorDaemon.h"
#include "MulticoreTaskExecutor.h"
#include <simgrid/msg.h>
#include <simgrid_util/DaemonWithMailbox.h>
#include <simgrid_util/Host.h>
#include <simgrid_util/Message.h>
#include <simgrid_util/Computation.h>
#include <simgrid_util/Clock.h>
#include <simgrid_util/Mailbox.h>
#include <exception/WRENCHException.h>
#include <compute_services/sequential_task_executor/SequentialTaskExecutor.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(multicore_task_executor_daemon, "Log category for Multicore Task Executor Daemon");

namespace WRENCH {

		/**
		 * @brief Constructor
		 */
		MulticoreTaskExecutorDaemon::MulticoreTaskExecutorDaemon(
						std::vector<SequentialTaskExecutor *> executors,
						ComputeService *cs): DaemonWithMailbox("multicore_task_executor", "multicore_task_executor") {

			this->compute_service = cs;

			// Initialize the set of idle executors (cores)
			this->idle_sequential_task_executors = {};
			for (int i=0; i < executors.size(); i++) {
				this->idle_sequential_task_executors.insert(executors[i]);
			}
			// Initialize the set of busy executors (cores)
			this->busy_sequential_task_executors = {};
		}

		/**
		 * @brief Destructor
		 */
		MulticoreTaskExecutorDaemon::~MulticoreTaskExecutorDaemon() {
		}

		/**
		 * @brief main() method of the daemon
		 *
		 * @return 0 on termination
		 */
		int MulticoreTaskExecutorDaemon::main() {
			XBT_INFO("New Multicore Task Executor starting (%s) with %ld cores ",
							   this->mailbox.c_str(), this->idle_sequential_task_executors.size());


			std::map<WorkflowTask*, std::string> callback_mailboxes;

			bool keep_going = true;
			while(keep_going) {

				// Wait for a message
				std::unique_ptr<Message> message = Mailbox::get(this->mailbox);

				// Process the message
				switch(message->type) {

					/** I should terminate **/
					case Message::STOP_DAEMON: {
						keep_going = false;
						break;
					}

					/** I was asked to run a task **/
					case Message::RUN_TASK: {
						std::unique_ptr<RunTaskMessage> m(static_cast<RunTaskMessage *>(message.release()));

						XBT_INFO("Asked to run task %s", m->task->id.c_str());

						// Just add the task to the task queue
						this->task_queue.push(m->task);

						// Remember the callback mailbox
						callback_mailboxes[m->task] = m->callback_mailbox;
						break;
					}

					/** One of my cores finished a task **/
					case Message::TASK_DONE: {
						std::unique_ptr<TaskDoneMessage> m(static_cast<TaskDoneMessage *>(message.release()));

						XBT_INFO("One of my cores completed task %s", m->task->id.c_str());

						// Put that core's executor back into the pull of idle cores
						SequentialTaskExecutor *executor = (SequentialTaskExecutor *)(m->compute_service);
						this->busy_sequential_task_executors.erase(executor);
						this->idle_sequential_task_executors.insert(executor);

						// Send the callback to the originator
						std::string callback_mailbox = callback_mailboxes[m->task];
						Mailbox::iput(callback_mailbox, new TaskDoneMessage(m->task, this->compute_service));
						break;

					}

					default: {
						throw WRENCHException("Unknown message type");
					}

				}

				// Run tasks while possible
				while ((task_queue.size() > 0)  && (idle_sequential_task_executors.size() > 0)) {
					// Get the task to run the first task on the first idle core
					WorkflowTask *to_run = task_queue.front();
					task_queue.pop();

					// Get an idle executor and make it busy
					SequentialTaskExecutor *executor = *(this->idle_sequential_task_executors.begin());
					this->idle_sequential_task_executors.erase(executor);
					this->busy_sequential_task_executors.insert(executor);

					XBT_INFO("Running task %s on one of my cores", to_run->id.c_str());
					executor->runTask(to_run, this->mailbox);
				}

			}

			XBT_INFO("Multicore Task Executor Daemon on host %s terminated!", Host::getHostName().c_str());
			return 0;
		}

};