/**
 *  @brief WRENCH::ComputeService is a mostly abstract implementation of a compute service.
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

};