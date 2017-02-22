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
#include "../compute_services/sequential_task_executor/SequentialTaskExecutor.h"
#include <simgrid/msg.h>

namespace WRENCH {

		/******************************/
		/**      PRIVATE METHODS     **/
		/******************************/


		/******************************/
		/**      PUBLIC METHODS     **/
		/******************************/

		/**
		 * @brief  Constructor
		 *
		 * @param filename is the path to a XML SimGrid platform description file
		 */
		Platform::Platform(std::string filename) {

			MSG_create_environment(filename.c_str());
		}

		/**
		 * @brief  Destructor
		 *
		 */
		Platform::~Platform() {

		}

		/**
		 * @brief method to instantiate a sequential task executor on a host
		 *
		 * @param hostname is the name of the host in the physical platform
		 */
		void Platform::addSequentialTaskExecutor(std::string hostname) {

			// Create the compute service
			std::shared_ptr<SequentialTaskExecutor> executor;
			try {
				 executor = std::make_shared<SequentialTaskExecutor>(hostname);
			} catch (Exception e) {
				throw e;
			}

			// Add it to the list of Compute Services
			compute_services.push_back(executor);

		}


};