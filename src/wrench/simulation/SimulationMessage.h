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

#ifndef WRENCH_SIMGRIDMESSAGES_H
#define WRENCH_SIMGRIDMESSAGES_H

#include <string>
#include <compute_services/ComputeService.h>
#include "workflow/WorkflowTask.h"

namespace wrench {

		// Base struct
		struct SimulationMessage {

				// Message type enum
				enum Type {
						STOP_DAEMON,
						RUN_TASK,
						TASK_DONE,
						TASK_FAILED
				};

				SimulationMessage(Type t, double s);

				Type type;
				double size;
		};

		// Derived struct
		struct StopDaemonMessage: public SimulationMessage {
				StopDaemonMessage();
		};

		// Derived struct
		struct RunTaskMessage: public SimulationMessage {
				RunTaskMessage(WorkflowTask*);
				WorkflowTask *task;
		};

		// Derived struct
		struct TaskDoneMessage: public SimulationMessage {
				TaskDoneMessage(WorkflowTask *, ComputeService*);
				WorkflowTask *task;
				ComputeService *compute_service;
		};

		// Derived struct
		struct TaskFailedMessage: public SimulationMessage {
				TaskFailedMessage(WorkflowTask *, ComputeService*);
				WorkflowTask *task;
				ComputeService *compute_service;
		};


};

#endif //WRENCH_SIMGRIDMESSAGES_H
