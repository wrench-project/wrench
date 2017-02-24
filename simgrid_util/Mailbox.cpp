//
// Created by Henri Casanova on 2/22/17.
//

#include "Mailbox.h"
#include <simgrid/msg.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(mailbox, "Log category for Mailbox");

namespace WRENCH {

		Mailbox::Mailbox() {
		}

		Mailbox::~Mailbox() {
		}

		Message *Mailbox::get(std::string mailbox) {
			msg_task_t msg_task = NULL;
			if (MSG_task_receive(&msg_task, mailbox.c_str())) {
				XBT_INFO("MAILBOX.GET ERROR");
				return nullptr;
			}

			Message *message = (Message *)MSG_task_get_data(msg_task);
			MSG_task_destroy(msg_task);
			return message;
		}

		void Mailbox::put(std::string mailbox, Message *m) {
			msg_task_t msg_task;
			msg_task = MSG_task_create("", 0, m->size, (void *)m);
			MSG_task_send(msg_task, mailbox.c_str());
			return;

		}

}