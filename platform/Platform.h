/**
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

namespace WRENCH {


		class Platform {

		public:
				Platform(std::string filename);

		};

};


#endif //WRENCH_PLATFORM_H
