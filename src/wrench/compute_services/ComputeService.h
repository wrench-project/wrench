/**
 *  @brief WRENCH::ComputeService implements an abstract compute service.
 */


#ifndef SIMULATION_COMPUTESERVICE_H
#define SIMULATION_COMPUTESERVICE_H


#include "workflow/WorkflowTask.h"

namespace WRENCH {

		class ComputeService {

		public:
				ComputeService(std::string);

				// Virtual methods to implement in derived classes
				virtual void stop() = 0;
				virtual int runTask(WorkflowTask *task, std::string callback_mailbox) = 0;

		private:
				std::string service_name;

		};
};


#endif //SIMULATION_COMPUTESERVICE_H
