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
#include <helper_daemons/sequential_task_executor/SequentialTaskExecutor.h>
#include "workflow_job/WorkflowJob.h"

namespace wrench {

		// Base struct
		struct SimulationMessage {

				// Message type enum
				enum Type {
						STOP_DAEMON,
						RUN_STANDARD_JOB,
						STANDARD_JOB_DONE,
						STANDARD_JOB_FAILED,
						RUN_TASK,
						TASK_DONE,
						NUM_IDLE_CORES_REQUEST,
						NUM_IDLE_CORES_ANSWER,
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
		struct RunJobMessage: public SimulationMessage {
				RunJobMessage(WorkflowJob*);
				WorkflowJob *job;
		};

		// Derived struct
		struct JobDoneMessage: public SimulationMessage {
				JobDoneMessage(WorkflowJob *, ComputeService*);
				WorkflowJob *job;
				ComputeService *compute_service;
		};

		// Derived struct
		struct JobFailedMessage: public SimulationMessage {
				JobFailedMessage(WorkflowJob *, ComputeService*);
				WorkflowJob *job;
				ComputeService *compute_service;
		};

		// Derived struct
		struct RunTaskMessage: public SimulationMessage {
				RunTaskMessage(WorkflowTask*);
				WorkflowTask *task;
		};

		// Derived struct
		struct TaskDoneMessage: public SimulationMessage {
				TaskDoneMessage(WorkflowTask *, SequentialTaskExecutor *);
				WorkflowTask *task;
				SequentialTaskExecutor *task_executor;
		};

		// Derived struct
		struct NumIdleCoresRequestMessage: public SimulationMessage {
				NumIdleCoresRequestMessage();
		};

		// Derived struct
		struct NumIdleCoresAnswerMessage: public SimulationMessage {
				NumIdleCoresAnswerMessage(unsigned long);
				unsigned long num_idle_cores;
		};

};

#endif //WRENCH_SIMGRIDMESSAGES_H
