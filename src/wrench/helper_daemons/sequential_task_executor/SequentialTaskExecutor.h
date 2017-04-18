/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef SIMULATION_SEQUENTIALTASKEXECUTOR_H
#define SIMULATION_SEQUENTIALTASKEXECUTOR_H

#include "workflow/WorkflowTask.h"

#include <memory>
#include <simgrid_S4U_util/S4U_DaemonWithMailbox.h>

namespace wrench {

		/**
		 *  @brief Implementation of a simple
 		 *  sequential task executor abstraction.
		 */
		class SequentialTaskExecutor : public S4U_DaemonWithMailbox {

		public:

				/** \cond INTERNAL **/

				/**
				 * @brief Constructor, which starts the daemon for the service on a host
				 *
				 * @param hostname: the name of the host
				 * @param callback_mailbox: the callback mailbox to which the sequential
				 *        task executor sends back "task done" or "task failed" messages
				 */
				SequentialTaskExecutor(std::string hostname, std::string callback_mailbox);

				/**
				 * @brief Terminate the sequential task executor
				 */
				void stop();

				/**
				 * @brief Kill the sequential task executor
				 */
				void kill();

				/**
		 		 * @brief Have the sequential task executor a task
		     *
		     * @param task: a pointer to the task
		     *
		     * @return 0 on success
		     */
				int runTask(WorkflowTask *task);

		private:

			 /**
	      * @brief Main method of the sequential task executor daemon
	      *
	      * @return 0 on termination
	      */
				int main();

				std::string callback_mailbox;
				std::string hostname;

				/** \endcond */
		};
};


#endif //SIMULATION_SEQUENTIALTASKEXECUTOR_H
