/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief S4U_Mailbox is a S4U wrapper
 */

#include <simulation/SimulationMessage.h>
#include <exception/WRENCHException.h>
#include "S4U_Mailbox.h"
#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(mailbox, "Mailbox");

namespace wrench {

		std::string S4U_Mailbox::generateUniqueMailboxName(std::string prefix) {
			static int unique_int = 0;
			return prefix + "_" + std::to_string(unique_int++);
		}

		/**
		 * @brief A blocking method to receive a message from a mailbox
		 *
		 * @param mailbox is the mailbox name
		 * @return a unique pointer to the message
		 */
		std::unique_ptr<SimulationMessage> S4U_Mailbox::get(std::string mailbox_name) {

//			XBT_INFO("GET from %s", mailbox_name.c_str());
			simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(mailbox_name);
		  SimulationMessage *msg = static_cast<SimulationMessage*>(simgrid::s4u::this_actor::recv(mailbox));
			if (msg == NULL) {
				throw WRENCHException("Mailbox::get(): NULL message in task");
			}

			return std::unique_ptr<SimulationMessage>(msg);
		}

		/**
		 * @brief A blocking method to send a message to a mailbox
		 *
		 * @param mailbox is the mailbox name
		 * @param m is the message
		 */
		void S4U_Mailbox::put(std::string mailbox_name, SimulationMessage *msg) {
//			XBT_INFO("PUT to %s", mailbox_name.c_str());
			simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(mailbox_name);
			simgrid::s4u::this_actor::send(mailbox, msg, (size_t)msg->size);

			return;
		}


};