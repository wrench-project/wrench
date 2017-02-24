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

		/**
		 * @brief A blocking method to receive a message from a mailbox
		 *
		 * @param mailbox is the mailbox name
		 * @return a unique pointer to the message
		 */
		std::unique_ptr<Message> Mailbox::get(std::string mailbox) {
			msg_task_t msg_task = NULL;
			if (MSG_task_receive(&msg_task, mailbox.c_str())) {
				XBT_INFO("MAILBOX.GET ERROR");
				return nullptr;
			}

			Message *message = (Message *)MSG_task_get_data(msg_task);
			MSG_task_destroy(msg_task);
			return std::unique_ptr<Message>(message);
		}

		/**
		 * @brief A blocking method to send a message to a mailbox
		 *
		 * @param mailbox is the mailbox name
		 * @param m is the message
		 */
		void Mailbox::put(std::string mailbox, Message *m) {
			msg_task_t msg_task;
			msg_task = MSG_task_create("", 0, m->size, (void *)m);
			MSG_task_send(msg_task, mailbox.c_str());
			return;

		}

		/**
		 * @brief A non-blocking method to send a message to a mailbox
		 *
		 * @param mailbox is the mailbox name
		 * @param m is the message
		 */
		void Mailbox::iput(std::string mailbox, Message *m) {
			msg_task_t msg_task;
			msg_task = MSG_task_create("", 0, m->size, (void *)m);
			MSG_task_isend(msg_task, mailbox.c_str());
			return;

		}

}