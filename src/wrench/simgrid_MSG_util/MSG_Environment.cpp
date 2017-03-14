/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
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

namespace wrench {

		void MSG_Environment::createEnvironmentFromXML(std::string filename) {
			// TODO: Raise exception is file is not found, since MSG_create_environment
			//       returns void
			MSG_create_environment(filename.c_str());
		}

};