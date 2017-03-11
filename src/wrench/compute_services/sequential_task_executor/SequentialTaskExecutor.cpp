/**
 *  @brief WRENCH::SequentialTaskExecutor class implements a simple
 *  sequential Compute Service abstraction.
 */

#include <simgrid_S4U_util/S4U_Mailbox.h>
#include "SequentialTaskExecutor.h"

namespace WRENCH {

		/**
		 * @brief Constructor, which starts the daemon for the service on a host
		 *
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
		 * @brief Terminate the sequential task executor
		 */
		void SequentialTaskExecutor::stop() {
			// Send a termination message to the daemon's mailbox
			S4U_Mailbox::iput(this->daemon->mailbox_name, new StopDaemonMessage());
		}

		/**
		 * @brief Have the sequential task executor execute a workflow task
		 *
		 * @param task is a pointer to the workflow task
		 * @param callback_mailbox is the name of a mailbox to which a "task done" callback will be sent
		 *
		 * @return 0 on success
		 */
		int SequentialTaskExecutor::runTask(WorkflowTask *task, std::string callback_mailbox) {

			// Asynchronously send a "run a task" message to the daemon's mailbox
			S4U_Mailbox::put(this->daemon->mailbox_name, new RunTaskMessage(task, callback_mailbox));
			return 0;
		};


}
