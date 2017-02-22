//
// Created by Henri Casanova on 2/22/17.
//

#include <iostream>
#include <simgrid/msg.h>


#include "SimpleWMSDaemon.h"
#include "../../simgrid_util/SimgridMessages.h"
#include "../../simgrid_util/SimgridMailbox.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simple_wms_daemon, "Log category for Simple WMS Daemon");

namespace WRENCH {

		SimpleWMSDaemon::SimpleWMSDaemon(Platform *p, Workflow *w): DaemonWithMailbox("simple_wms_daemon") {
			this->platform = p;
			this->workflow = w;
		}

		SimpleWMSDaemon::~SimpleWMSDaemon() {

		}


		int SimpleWMSDaemon::main() {
			XBT_INFO("New Simple WMS Daemon started on host %s listening on mailbox %s", MSG_host_get_name(MSG_host_self()), this->mailbox.c_str());


			XBT_INFO("Simple WMS Daemon started on host %s terminating", MSG_host_get_name(MSG_host_self()));
			return 0;
		}

};