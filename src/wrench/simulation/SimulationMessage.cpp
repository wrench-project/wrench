/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::SimulationMessage and derived classes to encapsulate
 *  control/data messages exchanged by simulated processes
 */

#include "SimulationMessage.h"

namespace wrench {

		/** Base Simgrid Message **/
		SimulationMessage::SimulationMessage(Type  t, double s) {
			type = t;
			size = s;
		}

		/** STOP_DAEMON MESSAGE **/
		// TODO: MAke the "1024" below configurable somehow
		StopDaemonMessage::StopDaemonMessage(): SimulationMessage(STOP_DAEMON, 1024.00) {
		}

		/** RUN_TASK MESSAGE **/
		// TODO: MAke the "1024" below configurable somehow
		RunTaskMessage::RunTaskMessage(WorkflowTask *t, std::string cb): SimulationMessage(RUN_TASK, 1024.0) {
			this->task = t;
			this->callback_mailbox = cb;
		}

		/** TASK_DONE MESSAGE **/
		// TODO: MAke the "1024" below configurable somehow
		TaskDoneMessage::TaskDoneMessage(WorkflowTask *t, ComputeService *cs): SimulationMessage(TASK_DONE, 1024.0) {
			this->task = t;
			this->compute_service = cs;
		}

};
