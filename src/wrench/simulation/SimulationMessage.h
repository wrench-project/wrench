/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
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

		/***********************/
		/** \cond INTERNAL     */
		/***********************/

		/**
		 * @brief Generic class that describe a message communicated by simulated processes
		 */
		class SimulationMessage {

		public:
				// Message type enum
				enum Type {
						STOP_DAEMON,
						DAEMON_STOPPED,
						RUN_STANDARD_JOB,
						STANDARD_JOB_DONE,
						STANDARD_JOB_FAILED,
						RUN_PILOT_JOB,
						PILOT_JOB_STARTED,
						PILOT_JOB_EXPIRED,
						PILOT_JOB_FAILED,
						RUN_TASK,
						TASK_DONE,
						NUM_IDLE_CORES_REQUEST,
						NUM_IDLE_CORES_ANSWER,
						TTL_REQUEST,
						TTL_ANSWER,
						JOB_TYPE_NOT_SUPPORTED,
						NOT_ENOUGH_CORES
				};

				SimulationMessage(Type t, double s);
				std::string toString();

				Type type;
				double size;
		};

		/**
		 * @brief "STOP_DAEMON" SimulationMessage class
		 */
		class StopDaemonMessage: public SimulationMessage {
		public:
				StopDaemonMessage(std::string ack_mailbox, double payload);
				std::string ack_mailbox;
		};

		/**
		 * @brief "DAEMON_STOPPED" SimulationMessage class
		 */
		class DaemonStoppedMessage: public SimulationMessage {
		public:
				DaemonStoppedMessage(double payload);
		};

		/**
		 * @brief "JOB_TYPE_NOT_SUPPORTED" SimulationMessage class
		 */
		class JobTypeNotSupportedMessage: public SimulationMessage {
		public:
				JobTypeNotSupportedMessage(WorkflowJob*, ComputeService *, double payload);
				WorkflowJob *job;
				ComputeService *compute_service;
		};

		/**
		 * @brief "NOT_ENOUGH_CORES" SimulationMessage class
		 */
		class NotEnoughCoresMessage: public SimulationMessage {
		public:
				NotEnoughCoresMessage(WorkflowJob*, ComputeService *, double payload);
				WorkflowJob *job;
				ComputeService *compute_service;
		};

		/**
		 * @brief "RUN_STANDARD_JOB" SimulationMessage class
		 */
		class RunStandardJobMessage: public SimulationMessage {
		public:
				RunStandardJobMessage(StandardJob*, double payload);
				StandardJob *job;
		};

		/**
		 * @brief "STANDARD_JOB_DONE" SimulationMessage class
		 */
		class StandardJobDoneMessage: public SimulationMessage {
		public:
				StandardJobDoneMessage(StandardJob *, ComputeService*, double payload);
				StandardJob *job;
				ComputeService *compute_service;
		};

		/**
		 * @brief "STANDARD_JOB_FAILED" SimulationMessage class
		 */
		class StandardJobFailedMessage: public SimulationMessage {
		public:
				StandardJobFailedMessage(StandardJob *, ComputeService*, double payload);
				StandardJob *job;
				ComputeService *compute_service;
		};

		/**
		 * @brief "RUN_PILOT_JOB" SimulationMessage class
		 */
		class RunPilotJobMessage: public SimulationMessage {
		public:
				RunPilotJobMessage(PilotJob*, double payload);
				PilotJob *job;
		};

		/**
		 * @brief "PILOT_JOB_STARTED" SimulationMessage class
		 */
		class PilotJobStartedMessage: public SimulationMessage {
		public:
				PilotJobStartedMessage(PilotJob *, ComputeService*, double payload);
				PilotJob *job;
				ComputeService *compute_service;
		};

		/**
		 * @brief "PILOT_JOB_EXPIRED" SimulationMessage class
		 */
		class PilotJobExpiredMessage: public SimulationMessage {
		public:
				PilotJobExpiredMessage(PilotJob *, ComputeService*, double payload);
				PilotJob *job;
				ComputeService *compute_service;
		};

		/**
		 * @brief "PILOT_JOB_FAILED" SimulationMessage class
		 */
		class PilotJobFailedMessage: public SimulationMessage {
		public:
				PilotJobFailedMessage(PilotJob *, ComputeService*, double payload);
				PilotJob *job;
				ComputeService *compute_service;
		};

		/**
		 * @brief "RUN_TASK" SimulationMessage class
		 */
		class RunTaskMessage: public SimulationMessage {
		public:
				RunTaskMessage(WorkflowTask*, double payload);
				WorkflowTask *task;
		};

		/**
		 * @brief "TASK_DONE" SimulationMessage class
		 */
		class TaskDoneMessage: public SimulationMessage {
		public:
				TaskDoneMessage(WorkflowTask *, SequentialTaskExecutor *, double payload);
				WorkflowTask *task;
				SequentialTaskExecutor *task_executor;
		};

		/**
		 * @brief "NUM_IDLE_CORES_REQUEST" SimulationMessage class
		 */
		class NumIdleCoresRequestMessage: public SimulationMessage {
		public:
				NumIdleCoresRequestMessage(double payload);
		};

		/**
		 * @brief "NUM_IDLE_CORES_ANSWER" SimulationMessage class
		 */
		class NumIdleCoresAnswerMessage: public SimulationMessage {
		public:
				NumIdleCoresAnswerMessage(unsigned int num, double payload);
				unsigned int num_idle_cores;
		};

		/**
		 * @brief "TTL_REQUEST" SimulationMessage class
		 */
		class TTLRequestMessage: public SimulationMessage {
		public:
				TTLRequestMessage(double payload);
		};

		/**
		 * @brief "TTL_ANSWER" SimulationMessage class
		 */
		class TTLAnswerMessage: public SimulationMessage {
		public:
				TTLAnswerMessage(double ttl, double payload);
				double ttl;
		};

		/***********************/
		/** \endcond           */
		/***********************/

};

#endif //WRENCH_SIMGRIDMESSAGES_H
