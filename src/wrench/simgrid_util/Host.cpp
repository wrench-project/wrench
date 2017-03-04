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
			msg_host_t host = MSG_host_self();
			if (host == NULL) {
				throw WRENCHException("Host::getHostName(): Can't get local host name");
			}
			return std::string(MSG_host_get_name(host));
		}

}