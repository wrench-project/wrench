/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @file    Mailbox.cpp
 *  @author  Henri Casanova
 *  @date    2/21/2017
 *  @version 1.0
 *
 *  @brief WRENCH::Mailbox class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::Mailbox class is a MSG wrapper
 *
 */


#include "MSG_Mailbox.h"
#include "exception/WRENCHException.h"

#include <simgrid/msg.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_mailbox, "Log category for Mailbox");

namespace wrench {


		/**
		 * @brief A blocking method to receive a message from a mailbox
		 *
		 * @param mailbox is the mailbox name
		 * @return a unique pointer to the message
		 */
		std::unique_ptr<SimulationMessage> MSG_Mailbox::get(std::string mailbox) {
			msg_task_t msg_task = NULL;
			if (MSG_task_receive(&msg_task, mailbox.c_str())) {
				throw WRENCHException("Mailbox::get(): Could not receive task from mailbox");
			}

			SimulationMessage *message = (SimulationMessage *)MSG_task_get_data(msg_task);
			if (message == NULL) {
				throw WRENCHException("Mailbox::get(): NULL message in task");
			}
			if (MSG_task_destroy(msg_task)) {
				throw WRENCHException("Mailbox::get(): Can't destroy task");
			}
			return std::unique_ptr<SimulationMessage>(message);
		}

		/**
		 * @brief A blocking method to send a message to a mailbox
		 *
		 * @param mailbox is the mailbox name
		 * @param m is the message
		 */
		void MSG_Mailbox::put(std::string mailbox, SimulationMessage *m) {
			msg_task_t msg_task;
			msg_task = MSG_task_create("", 0, m->size, (void *)m);
			if (MSG_task_send(msg_task, mailbox.c_str()) != MSG_OK) {
				throw WRENCHException("Mailbox::put(): Can't sent task");
			}
			return;

		}

		/**
		 * @brief A non-blocking method to send a message to a mailbox. This
		 *        is a "fire and forget" method, meaning that there is no
		 *        provided method to check that the put() has completed.
		 *
		 * @param mailbox is the mailbox name
		 * @param m is the message
		 */
		void MSG_Mailbox::iput(std::string mailbox, SimulationMessage *m) {
			msg_task_t msg_task;
			msg_task = MSG_task_create("", 0, m->size, (void *)m);
			// Using a "fire and forget" dsend(), passing null as the "callback if failure", which
			// is probably good enough for now
			MSG_task_dsend(msg_task, mailbox.c_str(), MSG_Mailbox::iput_failure_handler);
			return;
		}

		void MSG_Mailbox::iput_failure_handler(void *arg) {
			throw WRENCHException("Mailbox::iput(): Couldn't send task");
		}
}