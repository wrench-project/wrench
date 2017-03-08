/**
 *  @file    SequentialTaskExecutor.cpp
 *  @author  Henri Casanova
 *  @date    2/24/2017
 *  @version 1.0
 *
 *  @brief WRENCH::SequentialTaskExecutor class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::SequentialTaskExecutor class implements a simple
 *  Compute Service abstraction.
 *
 */

#include "SequentialTaskExecutor.h"
#include "simgrid_util/Mailbox.h"

namespace WRENCH {


		/**
		 * @brief Constructor, which starts the daemon for the service on a host
		 * @param hostname is the name of the host
		 */
		SequentialTaskExecutor::SequentialTaskExecutor(std::string hostname) : ComputeService("sequential_task_executor") {
			this->hostname = hostname;

			// Create the daemon
			this->daemon = std::unique_ptr<SequentialTaskExecutorDaemon>(new SequentialTaskExecutorDaemon(this));
			// Start the daemon on the host
			this->daemon->start(this->hostname);
		}

		/**
		 * @brief Destructor
		 */
		SequentialTaskExecutor::~SequentialTaskExecutor() {

		}

		/**
		 * @brief Method to terminate the compute service
		 */
		void SequentialTaskExecutor::stop() {
			// Send a termination message to the daemon's mailbox
			Mailbox::put(this->daemon->mailbox, new StopDaemonMessage());
		}

		/**
		 * @brief Causes the service to execute a workflow task
		 *
		 * @param task is a pointer to the workflow task
		 * @param callback_mailbox is the name of a mailbox to which a "task done" callback will be sent
		 * @return 0 on success
		 */
		int SequentialTaskExecutor::runTask(WorkflowTask *task, std::string callback_mailbox) {

			// Asynchronously send a "run a task" message to the daemon's mailbox
			Mailbox::iput(this->daemon->mailbox, new RunTaskMessage(task, callback_mailbox));
			return 0;
		};



}