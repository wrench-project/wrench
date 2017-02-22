//
// Created by Henri Casanova on 2/21/17.
//

#ifndef SIMULATION_COMPUTESERVICE_H
#define SIMULATION_COMPUTESERVICE_H

#include <string>
#include <simgrid/msg.h>

#include "../simgrid_util/DaemonWithMailbox.h"
#include "../workflow/WorkflowTask.h"

namespace WRENCH {

		class ComputeService {

		public:
				ComputeService(std::string);
				virtual ~ComputeService();

				/* Virtual methods to implement in derived classes */
//				virtual int start() = 0;
//				virtual int stop() = 0;
				virtual int runTask(std::shared_ptr<WorkflowTask> task) = 0;

		private:
				std::string service_name;

		};
};


#endif //SIMULATION_COMPUTESERVICE_H
