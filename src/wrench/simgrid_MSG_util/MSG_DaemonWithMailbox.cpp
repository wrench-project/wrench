/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @file    DaemonWithMailbox.cpp
 *  @author  Henri Casanova
 *  @date    2/24/2017
 *  @version 1.0
 *
 *  @brief WRENCH::DaemonWithMailbox class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::DaemonWithMailbox class is a MSG wrapper for a running processes that listens on a particular mailbox
 *
 */

#include "MSG_DaemonWithMailbox.h"
#include "exception/WRENCHException.h"

#include <simgrid/msg.h>
#include <iostream>

namespace wrench {

		/**
		 * @brief Constructor
		 *
		 * @param service_name is a string to identify the service (doesn't have to be unique)
		 */

		MSG_DaemonWithMailbox::MSG_DaemonWithMailbox(std::string mailbox_prefix, std::string process_name) {
			this->mailbox = mailbox_prefix + "_mailbox_" + std::to_string(getNewUniqueNumber());
			this->process_name = process_name;
			MSG_function_register(this->mailbox.c_str(), this->main_stub);
		}

		/**
		 * @brief Default Constructor
		 */

		MSG_DaemonWithMailbox::~MSG_DaemonWithMailbox() {}

		/**
		 * @brief main-stub to circumvent the fact that one must provide a pointer to a static
		 *        method to SimGrid for instantiating a simulated process
		 * @param argc
		 * @param argv
		 * @return 0 on success
		 */
		int MSG_DaemonWithMailbox::main_stub(int argc, char **argv) {

			if (argc != 1) {
				throw WRENCHException("DaemonWithMailbox::main_stub(): Exactly one \"command-line\" argument required");
			}

			// This is a pretty bad/clever hack in which the main method, which must be static,
			// gets a pointer to the instance via command-line arguments, and overwriting it
			// with a bogus string that will be freed automatically by SimGrid!
			MSG_DaemonWithMailbox *simulated_service_object = (MSG_DaemonWithMailbox *) argv[0];
			std::cerr << "DaemonWithMailbox: " << (long)simulated_service_object << std::endl;
			std::cerr << "DaemonWithMailbox: " << simulated_service_object->process_name << std::endl;
			std::cerr << "DaemonWithMailbox: " << simulated_service_object->mailbox << std::endl;

			argv[0] = strdup("SimGrid can free me!");

			return simulated_service_object->main();
		}

		/**
		 * @brief Method to start an instance of the service on a host
		 */
		void MSG_DaemonWithMailbox::start(std::string hostname) {

			msg_host_t host = MSG_host_by_name(hostname.c_str());
			if (!host) {
				throw WRENCHException("Unknown host " + hostname);
			}

			int argc = 1;
			char **argv = (char **) calloc(sizeof(char *), (size_t) argc);
			/* Ugly Hack to pass the instance to the function */
			argv[0] = (char *) this;

			std::cerr << " CREATING PROCESS WITH FAKE ARGV " << std::endl;
			std::cerr.flush();
			std::cerr << " CREATING PROCESS WITH FAKE ARGV " << (long)(argv[0]) << std::endl;
			std::cerr << " CREATING PROCESS WITH FAKE ARGV " << std::endl;

			msg_process_t process = MSG_process_create_with_arguments(this->process_name.c_str(), this->main_stub, NULL, host,
																																argc, argv);
			if (!process) {
				throw WRENCHException("DaemonWithMailbox::start(): Cannot start process " + this->mailbox + " on host " + hostname);
			}

			return;
		}

		/**
		 * @brief This method is used to generate a unique number for each newly generated daemon,
		 *        which is typically used to make sure that mailbox names are unique.
		 *
		 * @return a unique number
		 */
		int MSG_DaemonWithMailbox::getNewUniqueNumber() {
			static unsigned long sequence_number = 0;
			return (sequence_number++);
		}
};