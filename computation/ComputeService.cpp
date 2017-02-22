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
		 * @brief Constructor
		 */
		 ComputeService::ComputeService(std::string service_name) : SimulatedService(service_name) {

		}

		/**
		 * @brief Destructor
		 */
		 ComputeService::~ComputeService() {

		}

		/**
		 * @brief The main routine of the service
		 */
		int ComputeService::main() {
			std::cerr << "IN MAIN OF A COMPUTE SERVICE " << this->service_name << " ON HOST " << MSG_host_get_name(MSG_host_self()) <<
					" listening on mailbox " << this->mailbox << std::endl;
			return 0;
		}


};