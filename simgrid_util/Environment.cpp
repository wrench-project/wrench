/**
 *  @file    Environment.cpp
 *  @author  Henri Casanova
 *  @date    2/21/2017
 *  @version 1.0
 *
 *  @brief WRENCH::Environment class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::Environment class is a MSG wrapper
 *
 */


#include "Environment.h"

#include <simgrid/msg.h>

namespace WRENCH {

		void Environment::createEnvironmentFromXML(std::string filename) {
			MSG_create_environment(filename.c_str());
		}

};