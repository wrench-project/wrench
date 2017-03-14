/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::ComputeService is a mostly abstract implementation of a compute service.
 */

#include "ComputeService.h"


namespace wrench {

		/**
		 * @brief Constructor
		 *
		 * @param service_name is the name of the compute service
		 */
		ComputeService::ComputeService(std::string service_name) {
			this->service_name = service_name;
		}

};