/**
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


#include "Host.h"
#include "exception/WRENCHException.h"
#include <simgrid/msg.h>

namespace WRENCH {

		std::string Host::getHostName() {
			return std::string(MSG_host_get_name(get_local_host()));
		}

		int Host::getNumCores() {
			return MSG_host_get_core_number(get_local_host());
		}

		msg_host_t Host::get_local_host() {
			msg_host_t host = MSG_host_self();
			if (host == NULL) {
				throw WRENCHException("Host::getHostName(): Can't get local host name");
			}
			return host;
		}


}