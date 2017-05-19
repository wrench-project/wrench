/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <xbt/ex.hpp>
#include <simgrid/s4u/Mailbox.hpp>
#include <simgrid/s4u/Comm.hpp>

#include <simulation/SimulationMessage.h>
#include <logging/TerminalOutput.h>
#include <iostream>
#include "S4U_Mailbox.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(mailbox, "Mailbox");


namespace wrench {

		class WorkflowTask;

		// A data structure to keep track of pending asynchronous put() operations
		// what will have to be waited on at some point
		std::map<simgrid::s4u::ActorPtr, std::set<simgrid::s4u::Comm *>> S4U_Mailbox::dputs;

		/**
		 * @brief A method to generate a unique mailbox name give a prefix (this method
		 *        simply appends an increasing sequence number to the prefix)
		 *
		 * @param prefix: a prefix for the mailbox name
		 *
		 * @return a unique mailbox name as a string
		 */
		std::string S4U_Mailbox::generateUniqueMailboxName(std::string prefix) {
			static unsigned long sequence_number = 0;
			return prefix + "_" + std::to_string(sequence_number++);
		}

		/**
		 * @brief A blocking method to receive a message from a mailbox
		 *
		 * @param mailbox: the mailbox name
		 *
		 * @return a unique pointer to the message
		 *
		 * @throw std::runtime_error
		 */
		std::unique_ptr<SimulationMessage> S4U_Mailbox::get(std::string mailbox_name) {
			WRENCH_DEBUG("IN GET from %s", mailbox_name.c_str());
			simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(mailbox_name);
			SimulationMessage *msg = static_cast<SimulationMessage *>(simgrid::s4u::this_actor::recv(mailbox));
			if (msg == NULL) {
				throw std::runtime_error("NULL message received");
			}
			WRENCH_DEBUG("GOT a '%s' message from %s", msg->getName().c_str(), mailbox_name.c_str());
			return std::unique_ptr<SimulationMessage>(msg);
		}

		/**
		 * @brief A blocking method to receive a message from a mailbox, with a timeout
		 *
		 * @param mailbox: the mailbox name
		 * @param timeout:  a timeout value in seconds
		 *
		 * @return a unique pointer to the message, nullptr on timeout
		 *
		 * @throw std::runtime_error
		 */
		std::unique_ptr<SimulationMessage> S4U_Mailbox::get(std::string mailbox_name, double timeout) {
			WRENCH_DEBUG("IN GET WITH TIMEOUT (%lf) FROM MAILBOX %s", timeout, mailbox_name.c_str());
			simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(mailbox_name);
			void *data = nullptr;
			try {
				simgrid::s4u::Comm &comm = simgrid::s4u::this_actor::irecv(mailbox, &data);
				comm.wait(timeout);
			} catch (xbt_ex &e) {
				if (e.category == timeout_error) {
					return nullptr;
				}
			}

			if (data == nullptr) {
				throw std::runtime_error("NULL message in task");
			}

			SimulationMessage *msg = static_cast<SimulationMessage *>(data);

			WRENCH_DEBUG("GOT a '%s' message from %s", msg->getName().c_str(), mailbox_name.c_str());

			return std::unique_ptr<SimulationMessage>(msg);
		}

		/**
		 * @brief A blocking method to send a message to a mailbox
		 *
		 * @param mailbox: the mailbox name
		 * @param m: the SimulationMessage
		 */
		void S4U_Mailbox::put(std::string mailbox_name, SimulationMessage *msg) {
			WRENCH_DEBUG("PUTTING to %s a %s message", mailbox_name.c_str(), msg->getName().c_str());
			simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(mailbox_name);
			simgrid::s4u::this_actor::send(mailbox, msg, (size_t) msg->payload);

			return;
		}

		/**
		 * @brief A non-blocking method to send a message to a mailbox
		 *
		 * @param mailbox: the mailbox name
		 * @param m: the SimulationMessage
		 */
		void S4U_Mailbox::dput(std::string mailbox_name, SimulationMessage *msg) {

			WRENCH_DEBUG("DPUTTING to %s a %s message", mailbox_name.c_str(), msg->getName().c_str());

			simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(mailbox_name);
			simgrid::s4u::Comm &comm = simgrid::s4u::this_actor::isend(mailbox, msg, (int) msg->payload);

			// Insert the communication into the dputs map, so that it's not lost
			// and it can be "cleared" later
			S4U_Mailbox::dputs[simgrid::s4u::Actor::self()].insert(&comm);
			return;
		}

		/**
		 * @brief A method that checks on and clears previous asynchronous communications. This is
		 * to avoid having the above levels deal with asynchronous communication stuff.
		 */
		void S4U_Mailbox::clear_dputs() {
			std::set<simgrid::s4u::Comm *> set = S4U_Mailbox::dputs[simgrid::s4u::Actor::self()];
			std::set<simgrid::s4u::Comm *>::iterator it;
			for (it = set.begin(); it != set.end(); ++it) {
				// TODO: This is probably not great right now, but S4U asynchronous communication are
				// in a state of flux, and so this seems to work but for the memory leak
				// will have to talk to the S4U developers

//				XBT_INFO("Getting the state of a previous communication! (%s)", simgrid::s4u::Actor::self()->name().c_str());
				e_s4u_activity_state_t state = (*it)->getState();
				if (state == finished) {
//					XBT_INFO("The communication is finished.... remove it from the pending list [TODO: delete memory??? call test()???]");
					set.erase(*it);
				} else {
//					XBT_INFO("State = %d (finished = %d)", state, finished);
				}
			}
			return;
		}

		/**
		 * @brief Get the private mailbox name of a S4U actor
		 * @return the mailbox name
		 */
		std::string S4U_Mailbox::getPrivateMailboxName() {
			return "private_mailbox_" + simgrid::s4u::this_actor::name() + "_" + std::to_string(simgrid::s4u::this_actor::pid());
		}

};