/**
 *  @file    MulticoreTaskExecutor.cpp
 *  @author  Henri Casanova
 *  @date    3/7/2017
 *  @version 1.0
 *
 *  @brief WRENCH::MulticoreTaskExecutor class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::MulticoreTaskExecutor class implements a simple
 *  Compute Service abstraction.
 *
 */

#include <workflow/WorkflowTask.h>
#include <simgrid_util/Host.h>
#include "MulticoreTaskExecutor.h"
#include "simgrid_util/Mailbox.h"

namespace WRENCH {


		/**
		 * @brief Constructor, which starts the daemon for the service on a host
		 * @param hostname is the name of the host
		 */
		MulticoreTaskExecutor::MulticoreTaskExecutor(std::string hostname) : ComputeService("multicore_task_executor") {
			this->hostname = hostname;

			// Create and start one sequential task executor daemon per core
			int num_cores = Host::getNumCores(hostname);
			std::cerr << "num_cores = " << num_cores << std::endl;
			for (int i = 0; i < num_cores; i++) {
				std::cerr << "Creating a Sequential Task Executor" << std::endl;
				std::unique_ptr<SequentialTaskExecutor> seq_executor =
								std::unique_ptr<SequentialTaskExecutor>(new SequentialTaskExecutor(this->hostname));
				this->sequential_task_executors.push_back(std::move(seq_executor));
			}

			std::cerr << "Creating the main daemon" << std::endl;
			// Create the daemon
			std::vector<SequentialTaskExecutor *> executor_ptrs;
			for (int i = 0; i < this->sequential_task_executors.size(); i++) {
				executor_ptrs.push_back(this->sequential_task_executors[i].get());
			}
			this->daemon = std::unique_ptr<MulticoreTaskExecutorDaemon>(
							new MulticoreTaskExecutorDaemon(executor_ptrs, this));

			// Start the daemon on the host
			this->daemon->start(this->hostname);

		}

		/**
		 * @brief Destructor
		 */
		MulticoreTaskExecutor::~MulticoreTaskExecutor() {

		}

		/**
		 * @brief Method to terminate the compute service
		 */
		void MulticoreTaskExecutor::stop() {
			// Stop all sequential task executors
			for (auto &seq_exec : this->sequential_task_executors) {
				seq_exec->stop();

			}

			// Send a termination message to the daemon's mailbox
			Mailbox::put(this->daemon->mailbox, new StopDaemonMessage());
		}

		/**
		 * @brief Causes the service to execute a workflow task
		 *
		 * @param task is a pointer the workflow task
		 * @param callback_mailbox is the name of a mailbox to which a "task done" callback will be sent
		 * @return 0 on success
		 */
		int MulticoreTaskExecutor::runTask(WorkflowTask *task, std::string callback_mailbox) {

			// Asynchronously send a "run a task" message to the daemon's mailbox
			Mailbox::iput(this->daemon->mailbox, new RunTaskMessage(task, callback_mailbox));
			return 0;
		};


}