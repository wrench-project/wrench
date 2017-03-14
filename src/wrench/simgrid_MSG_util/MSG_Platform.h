/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @file    Platform.h
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

#ifndef WRENCH_PLATFORM_H
#define WRENCH_PLATFORM_H


#include <string>

namespace wrench {

		class MSG_Platform {

		public:
				MSG_Platform(std::string filename);
				~MSG_Platform();
		};

};


#endif //WRENCH_PLATFORM_H
