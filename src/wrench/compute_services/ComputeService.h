/**
 *  @file    ComputeService.h
 *  @author  Henri Casanova
 *  @date    2/24/2017
 *  @version 1.0
 *
 *  @brief WRENCH::ComputeService class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::ComputeService class is a mostly abstract implemenation of a compute service.
 *
 */


#ifndef SIMULATION_COMPUTESERVICE_H
#define SIMULATION_COMPUTESERVICE_H


#include "workflow/WorkflowTask.h"

namespace WRENCH {

		class ComputeService {

		public:
				ComputeService(std::string);

				virtual ~ComputeService();

				// Virtual methods to implement in derived classes
				virtual void stop() = 0;

				virtual int runTask(WorkflowTask *task, std::string callback_mailbox) = 0;

		private:
				std::string service_name;

		};
};


#endif //SIMULATION_COMPUTESERVICE_H
