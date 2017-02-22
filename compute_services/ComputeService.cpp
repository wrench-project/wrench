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
		 ComputeService::ComputeService(std::string service_name) {
			this->service_name = service_name;
		}

		/**
		 * @brief Destructor
		 */
		 ComputeService::~ComputeService() {

		}

};