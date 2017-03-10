//
// Created by Henri Casanova on 3/9/17.
//

#include "S4U_DaemonWithMailbox.h"
#include "S4U_DaemonWithMailboxActor.h"

#include <simgrid/s4u.hpp>

namespace WRENCH {

		S4U_DaemonWithMailbox::S4U_DaemonWithMailbox(std::string process_name, std::string mailbox_prefix) {
			static int unique_int = 0;
			this->process_name = process_name;
			this->mailbox_name = mailbox_prefix + "_" + std::to_string(unique_int++);
		}


		void S4U_DaemonWithMailbox::start(std::string hostname) {
			// Start the actor
			std::cerr << "Starting the actor..." << std::endl;
			this->actor = simgrid::s4u::Actor::createActor(this->process_name.c_str(),
																										 simgrid::s4u::Host::by_name("Tremblay"),
																										 S4U_DaemonWithMailboxActor(this));
		}

};