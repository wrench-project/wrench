//
// Created by Henri Casanova on 2/22/17.
//

#include <iostream>
#include <simgrid/msg.h>

#include "SimpleWMSDaemon.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simple_wms_daemon, "Log category for Simple WMS Daemon");

namespace WRENCH {

//		SimpleWMSDaemon::SimpleWMSDaemon(std::shared_ptr<Simulation> s, Workflow *w): DaemonWithMailbox("simple_wms_daemon") {
		SimpleWMSDaemon::SimpleWMSDaemon(Workflow *w): DaemonWithMailbox("simple_wms_daemon") {
//			this->simulation = s;
			this->workflow = w;
		}

		SimpleWMSDaemon::~SimpleWMSDaemon() {
		}


		int SimpleWMSDaemon::main() {
			XBT_INFO("Starting on host %s listening on mailbox %s", MSG_host_get_name(MSG_host_self()), this->mailbox.c_str());
			XBT_INFO("About to execute a workflow with %d tasks", this->workflow->getNumberOfTasks());
//			XBT_INFO("On a platform with %d sequential task executors", this->simulation->getNumberSequentialTaskExecutors());


			XBT_INFO("Simple WMS Daemon started on host %s terminating", MSG_host_get_name(MSG_host_self()));

			return 0;
		}

};