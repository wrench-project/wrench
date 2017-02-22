//
// Created by Henri Casanova on 2/22/17.
//

#include "SimgridMailbox.h"
#include <simgrid/msg.h>

namespace WRENCH {

		SimgridMailbox::SimgridMailbox() {
		}

		SimgridMailbox::~SimgridMailbox() {
		}

		SimgridMessage *SimgridMailbox::get(std::string mailbox) {
			msg_task_t msg_task = NULL;
			MSG_task_receive(&msg_task, mailbox.c_str());
			SimgridMessage *message = (SimgridMessage *)MSG_task_get_data(msg_task);
			MSG_task_destroy(msg_task);
			return message;
		}

		void SimgridMailbox::put(std::string mailbox, SimgridMessage *m) {
			msg_task_t msg_task;
			msg_task = MSG_task_create("", m->flops, m->bytes, (void *)m);
			MSG_task_send(msg_task, mailbox.c_str());
			return;

		}

}