/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @file    Environment.h
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

#ifndef SIMULATION_ENVIRONMENT_H
#define SIMULATION_ENVIRONMENT_H

#include <string>

namespace wrench {
		class MSG_Environment {
		public:
				static void createEnvironmentFromXML(std::string filename);
		};
}


#endif //SIMULATION_ENVIRONMENT_H
