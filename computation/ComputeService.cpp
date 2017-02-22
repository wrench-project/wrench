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
			MSG_function_register(service_name.c_str(), this->main_stub);
		}

		/**
		 * @brief Default Destructor
		 *
		 */
		ComputeService::~ComputeService() {

		}

		/**
		 *
		 */
		int ComputeService::main() {
			std::cerr << "IN MAIN OF A SERVICE " << this->service_name << " ON HOST " << MSG_host_get_name(MSG_host_self()) <<
					" listening on mailbox " << this->mailbox_name << std::endl;
			return 0;
		}


};