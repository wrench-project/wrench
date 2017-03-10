/**
 *  @file    Message.cpp
 *  @author  Henri Casanova
 *  @date    2/21/2017
 *  @version 1.0
 *
 *  @brief WRENCH::Message class implementations
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::Message and derived classes are MSG wrappers
 *
 */

#include "SimulationMessage.h"

namespace WRENCH {

		/** Base Simgrid Message **/
		SimulationMessage::SimulationMessage(Type  t, double s) {
			type = t;
			size = s;
		}
		SimulationMessage::~SimulationMessage() {

		}


		/** STOP_DAEMON MESSAGE **/
		// TODO: MAke the "1024" below configurable somehow
		StopDaemonMessage::StopDaemonMessage(): SimulationMessage(STOP_DAEMON, 1024.00) {
		}
		StopDaemonMessage::~StopDaemonMessage() {
		}

		/** RUN_TASK MESSAGE **/
		// TODO: MAke the "1024" below configurable somehow
		RunTaskMessage::RunTaskMessage(WorkflowTask *t, std::string cb): SimulationMessage(RUN_TASK, 1024.0) {
			this->task = t;
			this->callback_mailbox = cb;
		}
		RunTaskMessage::~RunTaskMessage() {
		}

		/** TASK_DONE MESSAGE **/
		// TODO: MAke the "1024" below configurable somehow
		TaskDoneMessage::TaskDoneMessage(WorkflowTask *t, ComputeService *cs): SimulationMessage(TASK_DONE, 1024.0) {
			this->task = t;
			this->compute_service = cs;
		}
		TaskDoneMessage::~TaskDoneMessage() {
		}




};
