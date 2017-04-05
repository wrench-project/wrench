/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief S4U_DaemonWithMailbox implements a generic "running daemon that
 *        listens on a mailbox" abstraction
 */

#include "S4U_DaemonWithMailbox.h"
#include "S4U_DaemonWithMailboxActor.h"
#include "S4U_Mailbox.h"

#include <simgrid/s4u.hpp>

namespace wrench {

	/**
	 * @brief Constructor
	 *
	 * @param process_name is the name of the simulated process/actor
	 * @param mailbox_prefix is the prefix of the mailbox (to which a unique integer is appended)
	 */
	S4U_DaemonWithMailbox::S4U_DaemonWithMailbox(std::string process_name, std::string mailbox_prefix) {
		this->process_name = process_name;
		this->mailbox_name = S4U_Mailbox::generateUniqueMailboxName(mailbox_prefix);
	}

	/**
	 * @brief Start the daemon
	 * @param hostname
	 */
	void S4U_DaemonWithMailbox::start(std::string hostname) {
		this->actor = simgrid::s4u::Actor::createActor(this->process_name.c_str(),
		                                               simgrid::s4u::Host::by_name(hostname),
		                                               S4U_DaemonWithMailboxActor(this));
		simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(this->mailbox_name);
		mailbox->setReceiver(this->actor);

	}

	/**
	 * @brief Kill the actor.
	 */
	void S4U_DaemonWithMailbox::kill_actor() {
		this->actor->kill();
	}

};