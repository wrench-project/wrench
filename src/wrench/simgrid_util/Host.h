/**
 *  @file    Host.cpp
 *  @author  Henri Casanova
 *  @date    2/24/2017
 *  @version 1.0
 *
 *  @brief WRENCH::Host class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::Host class is a MSG wrapper.
 *
 */


#ifndef SIMULATION_HOST_H
#define SIMULATION_HOST_H

#include <string>

namespace WRENCH {
		class Host {
		public:
				static std::string getHostName();

		};
};


#endif //SIMULATION_HOST_H
