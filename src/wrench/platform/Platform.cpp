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
#include "simgrid_MSG_util/MSG_Environment.h"


namespace WRENCH {

		/**
		 * @brief  Constructor
		 *
		 * @param filename is the path to a XML SimGrid platform description file
		 */
		Platform::Platform(std::string filename) {
			MSG_Environment::createEnvironmentFromXML(filename);
		}

		/**
		 * @brief  Destructor
		 *
		 */
		Platform::~Platform() {

		}



};