/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::SequentialTaskExecutor class implements a simple
 *  sequential task executor abstraction.
 */

#ifndef SIMULATION_SEQUENTIALTASKEXECUTOR_H
#define SIMULATION_SEQUENTIALTASKEXECUTOR_H

#include "workflow/WorkflowTask.h"

#include <memory>
#include <simgrid_S4U_util/S4U_DaemonWithMailbox.h>


namespace wrench {
		
	class SequentialTaskExecutor : public S4U_DaemonWithMailbox {

	public:
		SequentialTaskExecutor(std::string hostname, std::string callback_mailbox);
		void stop();
		void kill();
		int runTask(WorkflowTask *task);

	private:
			int main();
			std::string callback_mailbox;
			std::string hostname;
	};
};


#endif //SIMULATION_SEQUENTIALTASKEXECUTOR_H
