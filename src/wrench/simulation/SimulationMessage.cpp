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
		// TODO: Make the "1024" below configurable somehow
		StopDaemonMessage::StopDaemonMessage(): SimulationMessage(STOP_DAEMON, 1024.00) {
		}

		/** DAEMON_STOPPED MESSAGE **/
		// TODO: Make the "1024" below configurable somehow
		DaemonStoppedMessage::DaemonStoppedMessage(): SimulationMessage(DAEMON_STOPPED, 1024.00) {
		}

		/** RUN_JOB MESSAGE **/
		// TODO: Make the "1024" below configurable somehow
		RunStandardJobMessage::RunStandardJobMessage(StandardJob *job): SimulationMessage(RUN_STANDARD_JOB, 1024.0) {
			this->job = job;
		}

		/** JOB_DONE MESSAGE **/
		// TODO: Make the "1024" below configurable somehow
		StandardJobDoneMessage::StandardJobDoneMessage(StandardJob *job, ComputeService *cs): SimulationMessage(STANDARD_JOB_DONE, 1024.0) {
			this->job = job;
			this->compute_service = cs;
		}

		/** JOB_FAILED MESSAGE **/
		// TODO: Make the "1024" below configurable somehow
		StandardJobFailedMessage::StandardJobFailedMessage(StandardJob *job, ComputeService *cs): SimulationMessage(STANDARD_JOB_FAILED, 1024.0) {
			this->job = job;
			this->compute_service = cs;
		}

		/** RUN_PILOT_JOB **/
		// TODO: Make the "1024" below configurable somehow
		RunPilotJobMessage::RunPilotJobMessage(PilotJob *job): SimulationMessage(RUN_PILOT_JOB, 1024.0) {
			this->job = job;
		}

		/** PILOT_JOB_STARTED **/
		// TODO: Make the "1024" below configurable somehow
		PilotJobStartedMessage::PilotJobStartedMessage(PilotJob *job, ComputeService *cs): SimulationMessage(PILOT_JOB_STARTED, 1024.0) {
			this->job = job;
			this->compute_service = cs;
		}

		/** PILOT_JOB_TERMINATED **/
		// TODO: Make the "1024" below configurable somehow
		PilotJobTerminatedMessage::PilotJobTerminatedMessage(PilotJob *job, ComputeService *cs): SimulationMessage(PILOT_JOB_TERMINATED, 1024.0) {
			this->job = job;
			this->compute_service = cs;
		}

		/** PILOT_JOB_FAILED **/
		// TODO: Make the "1024" below configurable somehow
		PilotJobFailedMessage::PilotJobFailedMessage(PilotJob *job, ComputeService *cs): SimulationMessage(PILOT_JOB_FAILED, 1024.0) {
			this->job = job;
			this->compute_service = cs;
		}

		/** RUN_TASK MESSAGE **/
		// TODO: Make the "1024" below configurable somehow
		RunTaskMessage::RunTaskMessage(WorkflowTask *task): SimulationMessage(RUN_TASK, 1024.0) {
			this->task = task;
		}

		/** TASK_DONE MESSAGE **/
		// TODO: Make the "1024" below configurable somehow
		TaskDoneMessage::TaskDoneMessage(WorkflowTask *task, SequentialTaskExecutor *executor): SimulationMessage(TASK_DONE, 1024.0) {
			this->task = task;
			this->task_executor = executor;
		}

		/** NUM_IDLE_CORES_REQUEST MESSAGE **/
		// TODO: Make the "1024" below configurable somehow
		NumIdleCoresRequestMessage::NumIdleCoresRequestMessage() : SimulationMessage(NUM_IDLE_CORES_REQUEST, 1024.0) {
		}

		/** NUM_IDLE_CORES_ANSWER MESSAGE **/
		// TODO: Make the "1024" below configurable somehow
		NumIdleCoresAnswerMessage::NumIdleCoresAnswerMessage(unsigned long num) : SimulationMessage(NUM_IDLE_CORES_ANSWER, 1024.0) {
			this->num_idle_cores = num;
		}

};
