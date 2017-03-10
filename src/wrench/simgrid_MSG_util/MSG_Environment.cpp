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


#include "MSG_Environment.h"

#include <simgrid/msg.h>

namespace WRENCH {

		void MSG_Environment::createEnvironmentFromXML(std::string filename) {
			// TODO: Raise exception is file is not found, since MSG_create_environment
			//       returns void
			MSG_create_environment(filename.c_str());
		}

};