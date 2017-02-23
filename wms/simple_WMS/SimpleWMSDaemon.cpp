//
// Created by Henri Casanova on 2/22/17.
//

#include <iostream>
#include <simgrid/msg.h>

#include "SimpleWMSDaemon.h"
#include "../../simulation/Simulation.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simple_wms_daemon, "Log category for Simple WMS Daemon");

namespace WRENCH {

		SimpleWMSDaemon::SimpleWMSDaemon(Simulation *s, Workflow *w): DaemonWithMailbox("simple_wms_daemon") {
			this->simulation = s;
			this->workflow = w;
		}

		SimpleWMSDaemon::~SimpleWMSDaemon() {
		}


		int SimpleWMSDaemon::main() {
			XBT_INFO("Starting on host %s listening on mailbox %s", MSG_host_get_name(MSG_host_self()), this->mailbox.c_str());
			XBT_INFO("About to execute a workflow with %d tasks on a platform with %lu sequential executors",
							 this->workflow->getNumberOfTasks(),
							 this->simulation->getNumberSequentialTaskExecutors());

			XBT_INFO("Simple WMS Daemon is shutting everything down");
			this->simulation->shutdown();

			XBT_INFO("Simple WMS Daemon started on host %s terminating", MSG_host_get_name(MSG_host_self()));

			return 0;
		}

};