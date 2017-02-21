/**
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

#include "Platform.h"
#include <simgrid/msg.h>

namespace WRENCH {

		/******************************/
		/**      PRIVATE METHODS     **/
		/******************************/


		/******************************/
		/**      PUBLIC METHODS     **/
		/******************************/
		Platform::Platform(std::string filename) {
			MSG_create_environment(filename.c_str());
		}



};