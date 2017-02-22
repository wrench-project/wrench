//
// Created by Henri Casanova on 2/21/17.
//

#include "SimulatedService.h"
#include "../Exception/Exception.h"

#include <simgrid/msg.h>

namespace WRENCH {

		/**
		 * @brief Constructor
		 *
		 * @param service_name is a string to identify the service (doesn't have to be unique)
		 */

		SimulatedService::SimulatedService(std::string service_name) {
			this->service_name = service_name;
			this->mailbox = service_name + "_" + std::to_string(getNewUniqueNumber());
			MSG_function_register(service_name.c_str(), this->main_stub);
		}

		/**
		 * @brief Default Constructor
		 *
		 */

		SimulatedService::~SimulatedService() {}

		/**
		 * @brief main-stub to circumvent the fact that one must provide a pointer to a static
		 *        method to SimGrid for instantiating a simulated process
		 * @param argc
		 * @param argv
		 * @return 0 on success
		 */
		int SimulatedService::main_stub(int argc, char **argv) {

			if (argc != 1) {
				throw Exception("A simulated service stub for main() should take exactly one \"command-line\" argument");
			}


			// This is a pretty bad hack in which the main method gets, which must be static,
			// gets a pointer to the instance via command-line arguments, and overwriting it
			// with a bogus string that will be freed automatically by SimGrid!
			SimulatedService *simulated_service_object = (SimulatedService *)argv[0];
			argv[0] = strdup("SimGrid can free me!");

			return simulated_service_object->main();
		}

		/**
		 * @brief Method to start an instance of the service on a host
		 */
		void SimulatedService::start(std::string hostname) {
			static int sequence_number = 0;

			msg_host_t host = MSG_host_by_name(hostname.c_str());
			if (!host) {
				throw Exception("Unknown host " + hostname);
			}

			int argc = 1;
			char **argv = (char **)calloc(sizeof(char *), (size_t) argc);
			/* Ugly Hack to pass the instance to the function */
			argv[0] = (char *)this;

			msg_process_t process = MSG_process_create_with_arguments(this->service_name.c_str(), this->main_stub , NULL, host, argc, argv);
			if (!process) {
				throw Exception("Cannot start process " + this->service_name + " on host " + hostname);
			}

			return;
		}

		 int SimulatedService::getNewUniqueNumber() {
			static int number = 0;
			return (number++);
		}



};