/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @file    Platform.cpp
 *  @author  Henri Casanova
 *  @date    2/21/2017
 *  @version 1.0
 *
 *  @brief WRENCH::Platform class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::Platform class provides all basic functionality
 *  to represent/instantiate/manipulate a SimGrid simulation platform.
 *
 */

#include "MSG_Platform.h"
#include "simgrid_MSG_util/MSG_Environment.h"


namespace wrench {

		/**
		 * @brief  Constructor
		 *
		 * @param filename is the path to a XML SimGrid platform description file
		 */
		MSG_Platform::MSG_Platform(std::string filename) {
			MSG_Environment::createEnvironmentFromXML(filename);
		}

		/**
		 * @brief  Destructor
		 *
		 */
		MSG_Platform::~MSG_Platform() {

		}



};