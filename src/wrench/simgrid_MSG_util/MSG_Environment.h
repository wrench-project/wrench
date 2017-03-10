/**
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

namespace WRENCH {
		class MSG_Environment {
		public:
				static void createEnvironmentFromXML(std::string filename);
		};
}


#endif //SIMULATION_ENVIRONMENT_H
