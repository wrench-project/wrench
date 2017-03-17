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

#include <simgrid/s4u.hpp>

namespace wrench {

	/**
	 * @brief Constructor
	 *
	 * @param process_name is the name of the simulated process/actor
	 * @param mailbox_prefix is the prefix of the mailbox (to which a unique integer is appended)
	 */
	S4U_DaemonWithMailbox::S4U_DaemonWithMailbox(std::string process_name, std::string mailbox_prefix) {
		static int unique_int = 0;
		this->process_name = process_name;
		this->mailbox_name = mailbox_prefix + "_" + std::to_string(unique_int++);
	}

	/**
	 * @brief Start the daemon
	 * @param hostname
	 */
	void S4U_DaemonWithMailbox::start(std::string hostname) {
		this->actor = simgrid::s4u::Actor::createActor(this->process_name.c_str(),
		                                               simgrid::s4u::Host::by_name(hostname),
		                                               S4U_DaemonWithMailboxActor(this));
	}

	/**
	 * @brief Kill the actor. Warning, this will only kill is next times it places
	 *        a simcall. This means that if it's in the middle of computing, it will
	 *        finish the task, thus using (simulated) CPU resources. This is sort of like
	 *        a process that can only be killed when it stops computing... will be fixed in
	 *        S4U at some time.
	 */
	void S4U_DaemonWithMailbox::kill_actor() {
		this->actor->kill();
	}

};