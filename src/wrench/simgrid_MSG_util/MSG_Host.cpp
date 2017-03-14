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


#include "MSG_Host.h"
#include "exception/WRENCHException.h"
#include <simgrid/msg.h>

namespace wrench {

		std::string MSG_Host::getHostName() {
			msg_host_t local_host = MSG_host_self();
			if (local_host == NULL) {
				throw WRENCHException("Host::getHostName(): Can't get local host name");
			}
			return std::string(MSG_host_get_name(local_host));
		}

		int MSG_Host::getNumCores(std::string hostname) {
			return MSG_host_get_core_number(MSG_get_host_by_name(hostname.c_str()));
		}

}