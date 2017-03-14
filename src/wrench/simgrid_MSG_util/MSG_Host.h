/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @file    Host.cpp
 *  @author  Henri Casanova
 *  @date    2/24/2017
 *  @version 1.0
 *
 *  @brief WRENCH::Host class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::Host class is a MSG wrapper.
 *
 */


#ifndef SIMULATION_HOST_H
#define SIMULATION_HOST_H

#include <string>
#include <simgrid/msg.h>

namespace wrench {
		class MSG_Host {
		public:
				static std::string getHostName();
				static int getNumCores(std::string);
		private:
				static msg_host_t get_local_host();
		};
};


#endif //SIMULATION_HOST_H
