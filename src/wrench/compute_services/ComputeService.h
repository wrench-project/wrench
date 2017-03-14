/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::ComputeService implements an abstract compute service.
 */


#ifndef SIMULATION_COMPUTESERVICE_H
#define SIMULATION_COMPUTESERVICE_H


#include "workflow/WorkflowTask.h"

namespace wrench {

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
