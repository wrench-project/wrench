/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "SimulationMessage.h"

namespace wrench {


		/** Base Simgrid Message **/
		SimulationMessage::SimulationMessage(Type  t, double s) {
			type = t;
			size = s;
		}


		std::string SimulationMessage::toString() {
			switch(this->type) {
				case STOP_DAEMON: return "STOP_DAEMON";
				case DAEMON_STOPPED: return "DAEMON_STOPPED";
				case RUN_STANDARD_JOB: return "RUN_STANDARD_JOB";
				case STANDARD_JOB_DONE: return "STANDARD_JOB_DONE";
				case STANDARD_JOB_FAILED:return "STANDARD_JOB_FAILED";
				case RUN_PILOT_JOB:return "RUN_PILOT_JOB";
				case PILOT_JOB_STARTED:return "PILOT_JOB_STARTED";
				case PILOT_JOB_EXPIRED:return "PILOT_JOB_EXPIRED";
				case PILOT_JOB_FAILED:return "PILOT_JOB_FAILED";
				case RUN_TASK:return "RUN_TASK";
				case TASK_DONE:return "TASK_DONE";
				case NUM_IDLE_CORES_REQUEST:return "NUM_IDLE_CORES_REQUEST";
				case NUM_IDLE_CORES_ANSWER:return "NUM_IDLE_CORES_ANSWER";
				case TTL_REQUEST:return "TTL_REQUEST";
				case TTL_ANSWER:return "TTL_ANSWER";
				case JOB_TYPE_NOT_SUPPORTED: return "JOB_TYPE_NOT_SUPPORTED";
				case NOT_ENOUGH_CORES: return "NOT_ENOUGH_CORES";
				default: return "UNKNOWN MESSAGE TYPE";
			}

		}

		/**
		 * @brief Constructor
		 * @param payload: message size in bytes
		 */
		StopDaemonMessage::StopDaemonMessage(std::string ack_mailbox,
																				 double payload): SimulationMessage(STOP_DAEMON, payload) {
			this->ack_mailbox = ack_mailbox;
		}

		/**
		 * @brief Constructor
		 * @param payload: message size in bytes
		 */
		DaemonStoppedMessage::DaemonStoppedMessage(double payload): SimulationMessage(DAEMON_STOPPED, payload) {
		}

		/**
		 * @brief Constructor
		 * @param job: pointer to a WorkflowJob
		 * @param cs: points to a ComputeService
		 * @param payload: message size in bytes
		 */
		JobTypeNotSupportedMessage::JobTypeNotSupportedMessage(WorkflowJob *job, ComputeService *cs, double payload): SimulationMessage(JOB_TYPE_NOT_SUPPORTED, payload) {
			this->job = job;
			this->compute_service = cs;
		}

		/**
		 * @brief Constructor
		 * @param job: pointer to a WorkflowJob
		 * @param cs: points to a ComputeService
		 * @param payload: message size in bytes
		 */
		NotEnoughCoresMessage::NotEnoughCoresMessage(WorkflowJob *job, ComputeService *cs, double payload): SimulationMessage(NOT_ENOUGH_CORES, payload) {
			this->job = job;
			this->compute_service = cs;
		}

		/**
		 * @brief Constructor
		* @param job: pointer to a StandardJob
		 * @param payload: message size in bytes
		 */
		RunStandardJobMessage::RunStandardJobMessage(StandardJob *job, double payload): SimulationMessage(RUN_STANDARD_JOB, payload) {
			this->job = job;
		}

		/**
		 * @brief Constructor
		 * @param job: pointer to a StandardJob
		 * @param cs: points to a ComputeService
		 * @param payload: message size in bytes
		 */
		StandardJobDoneMessage::StandardJobDoneMessage(StandardJob *job, ComputeService *cs, double payload): SimulationMessage(STANDARD_JOB_DONE, payload) {
			this->job = job;
			this->compute_service = cs;
		}

		/**
		 * @brief Constructor
		 * @param job: pointer to a StandardJob
		 * @param cs: points to a ComputeService
		 * @param payload: message size in bytes
		 */
		StandardJobFailedMessage::StandardJobFailedMessage(StandardJob *job, ComputeService *cs, double payload): SimulationMessage(STANDARD_JOB_FAILED, payload) {
			this->job = job;
			this->compute_service = cs;
		}

		/**
		 * @brief Constructor
		 * @param job: pointer to a PilotJob
		 * @param payload: message size in bytes
		 */
		RunPilotJobMessage::RunPilotJobMessage(PilotJob *job, double payload): SimulationMessage(RUN_PILOT_JOB, payload) {
			this->job = job;
		}

		/**
		 * @brief Constructor
		 * @param job: pointer to a PilotJob
		 * @param cs: points to a ComputeService
		 * @param payload: message size in bytes
		 */
		PilotJobStartedMessage::PilotJobStartedMessage(PilotJob *job, ComputeService *cs, double payload): SimulationMessage(PILOT_JOB_STARTED, payload) {
			this->job = job;
			this->compute_service = cs;
		}

		/**
		 * @brief Constructor
		 * @param job: pointer to a PilotJob
		 * @param cs: points to a ComputeService
		 * @param payload: message size in bytes
		 */
		PilotJobExpiredMessage::PilotJobExpiredMessage(PilotJob *job, ComputeService *cs, double payload): SimulationMessage(PILOT_JOB_EXPIRED, payload) {
			this->job = job;
			this->compute_service = cs;
		}

		/**
		 * @brief Constructor
		 * @param job: pointer to a PilotJob
		 * @param cs: points to a ComputeService
		 * @param payload: message size in bytes
		 */
		PilotJobFailedMessage::PilotJobFailedMessage(PilotJob *job, ComputeService *cs, double payload): SimulationMessage(PILOT_JOB_FAILED, payload) {
			this->job = job;
			this->compute_service = cs;
		}

		/**
		 * @brief Constructor
		 * @param task: pointer to a WorkflowTask
		 * @param payload: message size in bytes
		 */
		RunTaskMessage::RunTaskMessage(WorkflowTask *task, double payload): SimulationMessage(RUN_TASK, payload) {
			this->task = task;
		}

		/**
		 * @brief Constructor
		 * @param task: pointer to a WorkflowTask
		 * @param executor: pointer to a SequentialTaskExecutor
		 * @param payload: message size in bytes
		 */		TaskDoneMessage::TaskDoneMessage(WorkflowTask *task, SequentialTaskExecutor *executor, double payload): SimulationMessage(TASK_DONE, payload) {
			this->task = task;
			this->task_executor = executor;
		}

		/**
		 * @brief Constructor
		 * @param payload: message size in bytes
		 */
		NumIdleCoresRequestMessage::NumIdleCoresRequestMessage(double payload) : SimulationMessage(NUM_IDLE_CORES_REQUEST, payload) {
		}

		/**
		 * @brief Constructor
		 * @param num: number of idle cores
		 * @param payload: message size in bytes
		 */
		NumIdleCoresAnswerMessage::NumIdleCoresAnswerMessage(unsigned int num, double payload) : SimulationMessage(NUM_IDLE_CORES_ANSWER, payload) {
			this->num_idle_cores = num;
		}

		/**
		 * @brief Constructor
		 * @param payload: message size in bytes
		 */
		TTLRequestMessage::TTLRequestMessage(double payload) : SimulationMessage(TTL_REQUEST, payload) {
		}

		/**
		 * @brief Constructor
		 * @param num: time-to-live, in seconds
		 * @param payload: message size in bytes
		 */
		TTLAnswerMessage::TTLAnswerMessage(double  ttl, double payload) : SimulationMessage(TTL_ANSWER, payload) {
			this->ttl = ttl;
		}

};
