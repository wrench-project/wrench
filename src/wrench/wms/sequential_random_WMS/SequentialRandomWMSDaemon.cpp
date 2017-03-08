/**
 *  @file    SequentialRandomWMSDaemon.cpp
 *  @author  Henri Casanova
 *  @date    2/24/2017
 *  @version 1.0
 *
 *  @brief WRENCH::SequentialRandomWMSDaemon class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::SequentialRandomWMSDaemon class implements the daemon for a simple WMS abstraction
 *
 */

#include <iostream>
#include <simgrid/msg.h>

#include "SequentialRandomWMSDaemon.h"
#include "simulation/Simulation.h"
#include "simgrid_util/Mailbox.h"
#include "simgrid_util/Host.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simple_wms_daemon, "Log category for Simple WMS Daemon");

namespace WRENCH {

		/**
		 * @brief Constructor
		 *
		 * @param s is a pointer to the simulation
		 * @param w is a pointer to the workflow to execute
		 */
		SequentialRandomWMSDaemon::SequentialRandomWMSDaemon(Simulation *s, Workflow *w): DaemonWithMailbox("simple_wms_daemon") {
			this->simulation = s;
			this->workflow = w;
		}

		/**
		 * @brief main method of the daemon
		 *
		 * @return 0 on completion
		 */
		int SequentialRandomWMSDaemon::main() {
			XBT_INFO("Starting on host %s listening on mailbox %s", Host::getHostName().c_str(), this->mailbox.c_str());
			XBT_INFO("About to execute a workflow with %lu tasks",this->workflow->getNumberOfTasks());

			/** Stupid Scheduling/Execution Algorithm **/

			while(true) {

				// Submit all ready tasks for execution
				std::vector<WorkflowTask*> ready_tasks = this->workflow->getReadyTasks();
				XBT_INFO("There are %ld ready tasks", ready_tasks.size());
				for (int i=0; i < ready_tasks.size(); i++) {
					XBT_INFO("Submitting task %s for execution", ready_tasks[i]->id.c_str());
					ready_tasks[i]->setScheduled();
					this->simulation->getSomeMulticoreTaskExecutor()->runTask(
									ready_tasks[i], this->mailbox);
				}

				// Wait for a task completion
				XBT_INFO("Waiting for a task to complete...");
				std::unique_ptr<Message> message = Mailbox::get(this->mailbox);
				std::unique_ptr<TaskDoneMessage> m(static_cast<TaskDoneMessage*>(message.release()));

				XBT_INFO("Notified that task %s has completed", m->task->id.c_str());

				if (workflow->isDone()) {
					break;
				}
			}

			XBT_INFO("Workflow execution is complete!");


			XBT_INFO("Simple WMS Daemon is shutting down all Compute Services");
			this->simulation->shutdown();

			XBT_INFO("Simple WMS Daemon started on host %s terminating", Host::getHostName().c_str());

			return 0;
		}

};