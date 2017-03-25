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

namespace wrench {

		class ComputeService;
		class WorkflowJob;
		class StandardJob;
		class PilotJob;

		// Base struct
		struct SimulationMessage {

				// Message type enum
				enum Type {
						STOP_DAEMON,
						DAEMON_STOPPED,
						RUN_STANDARD_JOB,
						STANDARD_JOB_DONE,
						STANDARD_JOB_FAILED,
						RUN_PILOT_JOB,
						PILOT_JOB_STARTED,
						PILOT_JOB_TERMINATED,
						PILOT_JOB_FAILED,
						RUN_TASK,
						TASK_DONE,
						NUM_IDLE_CORES_REQUEST,
						NUM_IDLE_CORES_ANSWER,
				};

				SimulationMessage(Type t, double s);

				Type type;
				double size;
		};

		/** DAEMON STOPPING **/
		struct StopDaemonMessage: public SimulationMessage {
				StopDaemonMessage();
		};

		struct DaemonStoppedMessage: public SimulationMessage {
				DaemonStoppedMessage();
		};

		/** STANDARD JOBS */
		struct RunStandardJobMessage: public SimulationMessage {
				RunStandardJobMessage(StandardJob*);
				StandardJob *job;
		};

		struct StandardJobDoneMessage: public SimulationMessage {
				StandardJobDoneMessage(StandardJob *, ComputeService*);
				StandardJob *job;
				ComputeService *compute_service;
		};

		struct StandardJobFailedMessage: public SimulationMessage {
				StandardJobFailedMessage(StandardJob *, ComputeService*);
				StandardJob *job;
				ComputeService *compute_service;
		};

		/** PILOT JOBS */
		struct RunPilotJobMessage: public SimulationMessage {
				RunPilotJobMessage(PilotJob*);
				PilotJob *job;
		};

		struct PilotJobStartedMessage: public SimulationMessage {
				PilotJobStartedMessage(PilotJob *, ComputeService*);
				PilotJob *job;
				ComputeService *compute_service;
		};

		struct PilotJobTerminatedMessage: public SimulationMessage {
				PilotJobTerminatedMessage(PilotJob *, ComputeService*);
				PilotJob *job;
				ComputeService *compute_service;
		};

		struct PilotJobFailedMessage: public SimulationMessage {
				PilotJobFailedMessage(PilotJob *, ComputeService*);
				PilotJob *job;
				ComputeService *compute_service;
		};

		/** TASKS **/

		struct RunTaskMessage: public SimulationMessage {
				RunTaskMessage(WorkflowTask*);
				WorkflowTask *task;
		};

		struct TaskDoneMessage: public SimulationMessage {
				TaskDoneMessage(WorkflowTask *, SequentialTaskExecutor *);
				WorkflowTask *task;
				SequentialTaskExecutor *task_executor;
		};

		/** NUM IDLE CORES QUERIES **/

		struct NumIdleCoresRequestMessage: public SimulationMessage {
				NumIdleCoresRequestMessage();
		};

		struct NumIdleCoresAnswerMessage: public SimulationMessage {
				NumIdleCoresAnswerMessage(unsigned long);
				unsigned long num_idle_cores;
		};

};

#endif //WRENCH_SIMGRIDMESSAGES_H
