//
// Created by Henri Casanova on 2/21/17.
//

#include "ComputeService.h"

#include <simgrid/msg.h>
#include <string>
#include <iostream>
#include "../Exception/Exception.h"

namespace WRENCH {

		/**
		 * @brief Constructor of the generic ComputeService class
		 *
		 * @param service_name is a string that describes the service name
		 */
		ComputeService::ComputeService(std::string service_name) {
			this->service_name = service_name;
			MSG_function_register(service_name.c_str(), this->main);
		}

		/**
		 * @brief Default Destructor
		 *
		 */
		ComputeService::~ComputeService() {

		}

		/**
		 * @brief main() STATIC method of a Compute Service
		 *
		 * @param argc is the number of "command-line" arguments
		 * @param argv is the list of "command-line" arguments
		 *
		 * @return
		 */
		int ComputeService::main(int argc, char **argv) {

			if (argc != 1) {
				throw Exception("Invalid command-line arguments!");
			}

			std::string mailbox_name = argv[0];

			std::cerr << "New service starting on host " << MSG_host_get_name(MSG_host_self()) << " at mailbox " << mailbox_name << std::endl;

			std::cerr << "Service stopping on host " << MSG_host_get_name(MSG_host_self()) << " at mailbox " << mailbox_name << std::endl;

			return 0;
		}

		/**
		 * @brief Method to start an instance of the service on a host
		 */
		void ComputeService::start(std::string hostname) {
			static int instance_count = 0;
			this->mailbox_name = this->service_name + "_" + hostname + "_" + std::to_string(instance_count++);

			msg_host_t host = MSG_host_by_name(hostname.c_str());
			if (!host) {
				throw Exception("Unknown host " + hostname);
			}

			int argc = 1;
			char **argv = (char **)calloc(sizeof(char *), (size_t) argc);
			argv[0] = strdup(this->mailbox_name.c_str());

			msg_process_t process = MSG_process_create_with_arguments(this->service_name.c_str(), ComputeService::main , NULL, host, argc, argv);
			if (!process) {
				throw Exception("Cannot start process " + this->service_name + " on host " + hostname);
			}

			std::cerr << "Called MSG_process_create_with_arguments!" << std::endl;
			return;
		}


};