/**
 *  @file    ComputeService.cpp
 *  @author  Henri Casanova
 *  @date    2/24/2017
 *  @version 1.0
 *
 *  @brief WRENCH::ComputeService class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::ComputeService class is a mostly abstract implementation of a compute service.
 *
 */

#include "ComputeService.h"


namespace WRENCH {

		/**
		 * @brief Constructor
		 *
		 * @param service_name is the name of the compute service
		 */
		ComputeService::ComputeService(std::string service_name) {
			this->service_name = service_name;
		}

		/**
		 * @brief Destructor
		 */
		ComputeService::~ComputeService() {

		}


};