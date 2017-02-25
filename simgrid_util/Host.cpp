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
#include <simgrid/msg.h>

namespace WRENCH {

		std::string Host::getHostName() {
			return std::string(MSG_host_get_name(MSG_host_self()));
		}

}