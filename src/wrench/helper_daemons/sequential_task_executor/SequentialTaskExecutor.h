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
#include "SequentialTaskExecutorDaemon.h"

#include <memory>


namespace wrench {
		
	class SequentialTaskExecutor {

	public:
		SequentialTaskExecutor(std::string hostname, std::string callback_mailbox);
		void stop();
		void kill();
		int runTask(WorkflowTask *task);

	private:
		std::string hostname;
		std::unique_ptr<SequentialTaskExecutorDaemon> daemon;

	};
};


#endif //SIMULATION_SEQUENTIALTASKEXECUTOR_H
