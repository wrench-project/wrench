//
// Created by Henri Casanova on 2/22/17.
//

#include <iostream>
#include <simgrid/msg.h>

#include "SequentialRandomWMSDaemon.h"
#include "../../simulation/Simulation.h"
#include "../../simgrid_util/Mailbox.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simple_wms_daemon, "Log category for Simple WMS Daemon");

namespace WRENCH {

		SequentialRandomWMSDaemon::SequentialRandomWMSDaemon(Simulation *s, Workflow *w): DaemonWithMailbox("simple_wms_daemon") {
			this->simulation = s;
			this->workflow = w;
		}

		SequentialRandomWMSDaemon::~SequentialRandomWMSDaemon() {
		}


		int SequentialRandomWMSDaemon::main() {
			XBT_INFO("Starting on host %s listening on mailbox %s", MSG_host_get_name(MSG_host_self()), this->mailbox.c_str());
			XBT_INFO("About to execute a workflow with %lu tasks on a platform with %lu sequential executors",
							 this->workflow->getNumberOfTasks(),
							 this->simulation->getNumberSequentialTaskExecutors());

			/** Stupid Scheduling/Execution Algorithm **/

			while(true) {

				// Look for a ready task
				std::shared_ptr<WorkflowTask> ready_task = this->workflow->getSomeReadyTask();

				// If none, we're done
				if (!ready_task) {
					break;
				}

				// Submit the task for execution
				XBT_INFO("Submitting task %s for execution", ready_task->id.c_str());
				this->simulation->getSomeSequentialTaskExecutor()->runTask(ready_task, this->mailbox);

				// Wait for its completion
				XBT_INFO("Waiting for task %s to complete...", ready_task->id.c_str());

				std::unique_ptr<Message> message = Mailbox::get(this->mailbox);
				std::unique_ptr<TaskDoneMessage> m(static_cast<TaskDoneMessage*>(message.release()));

				XBT_INFO("Notified that task %s has completed", m->task->id.c_str());

				// Update task state
				workflow->makeTaskCompleted(m->task);
			}

			XBT_INFO("Workflow execution is complete!");


			XBT_INFO("Simple WMS Daemon is shutting everything down");
			this->simulation->shutdown();

			XBT_INFO("Simple WMS Daemon started on host %s terminating", MSG_host_get_name(MSG_host_self()));

			return 0;
		}

};