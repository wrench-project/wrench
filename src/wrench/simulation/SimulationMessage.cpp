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

		/****************************/
		/**	INTERNAL METHODS BELOW **/
		/****************************/

		/*! \cond INTERNAL */

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
				default: return "UNKNOWN MESSAGE TYPE";
			}

		}

		/** STOP_DAEMON MESSAGE **/
		StopDaemonMessage::StopDaemonMessage(double payload): SimulationMessage(STOP_DAEMON, payload) {
		}

		/** DAEMON_STOPPED MESSAGE **/
		DaemonStoppedMessage::DaemonStoppedMessage(double payload): SimulationMessage(DAEMON_STOPPED, payload) {
		}

		/** JOB_TYPE_NOT_SUPPORTED MESSAGE **/
		JobTypeNotSupportedMessage::JobTypeNotSupportedMessage(WorkflowJob *job, ComputeService *cs, double payload): SimulationMessage(JOB_TYPE_NOT_SUPPORTED, payload) {
			this->job = job;
			this->compute_service = cs;
		}

		/** RUN_STANDARD_JOB MESSAGE **/
		RunStandardJobMessage::RunStandardJobMessage(StandardJob *job, double payload): SimulationMessage(RUN_STANDARD_JOB, payload) {
			this->job = job;
		}

		/** STANDARD_JOB_DONE MESSAGE **/
		StandardJobDoneMessage::StandardJobDoneMessage(StandardJob *job, ComputeService *cs, double payload): SimulationMessage(STANDARD_JOB_DONE, payload) {
			this->job = job;
			this->compute_service = cs;
		}

		/** STANDARD_JOB_FAILED MESSAGE **/
		StandardJobFailedMessage::StandardJobFailedMessage(StandardJob *job, ComputeService *cs, double payload): SimulationMessage(STANDARD_JOB_FAILED, payload) {
			this->job = job;
			this->compute_service = cs;
		}

		/** RUN_PILOT_JOB **/
		RunPilotJobMessage::RunPilotJobMessage(PilotJob *job, double payload): SimulationMessage(RUN_PILOT_JOB, payload) {
			this->job = job;
		}

		/** PILOT_JOB_STARTED **/
		PilotJobStartedMessage::PilotJobStartedMessage(PilotJob *job, ComputeService *cs, double payload): SimulationMessage(PILOT_JOB_STARTED, payload) {
			this->job = job;
			this->compute_service = cs;
		}

		/** PILOT_JOB_TERMINATED **/
		PilotJobExpiredMessage::PilotJobExpiredMessage(PilotJob *job, ComputeService *cs, double payload): SimulationMessage(PILOT_JOB_EXPIRED, payload) {
			this->job = job;
			this->compute_service = cs;
		}

		/** PILOT_JOB_FAILED **/
		PilotJobFailedMessage::PilotJobFailedMessage(PilotJob *job, ComputeService *cs, double payload): SimulationMessage(PILOT_JOB_FAILED, payload) {
			this->job = job;
			this->compute_service = cs;
		}

		/** RUN_TASK MESSAGE **/
		RunTaskMessage::RunTaskMessage(WorkflowTask *task, double payload): SimulationMessage(RUN_TASK, payload) {
			this->task = task;
		}

		/** TASK_DONE MESSAGE **/
		TaskDoneMessage::TaskDoneMessage(WorkflowTask *task, SequentialTaskExecutor *executor, double payload): SimulationMessage(TASK_DONE, payload) {
			this->task = task;
			this->task_executor = executor;
		}

		/** NUM_IDLE_CORES_REQUEST MESSAGE **/
		NumIdleCoresRequestMessage::NumIdleCoresRequestMessage(double payload) : SimulationMessage(NUM_IDLE_CORES_REQUEST, payload) {
		}

		/** NUM_IDLE_CORES_ANSWER MESSAGE **/
		NumIdleCoresAnswerMessage::NumIdleCoresAnswerMessage(unsigned int num, double payload) : SimulationMessage(NUM_IDLE_CORES_ANSWER, payload) {
			this->num_idle_cores = num;
		}

		/** TTL_REQUEST MESSAGE **/
		TTLRequestMessage::TTLRequestMessage(double payload) : SimulationMessage(TTL_REQUEST, payload) {
		}

		/** TTL_ANSWER MESSAGE **/
		TTLAnswerMessage::TTLAnswerMessage(double  ttl, double payload) : SimulationMessage(TTL_ANSWER, payload) {
			this->ttl = ttl;
		}

		/*! \endcond */

};
