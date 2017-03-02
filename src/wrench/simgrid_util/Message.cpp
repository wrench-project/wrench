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

#include "Message.h"

namespace WRENCH {

		/** Base Simgrid Message **/
		Message::Message(Type  t, double s) {
			type = t;
			size = s;
		}
		Message::~Message() {

		}


		/** STOP_DAEMON MESSAGE **/
		// TODO: MAke the "1024" below configurable somehow
		StopDaemonMessage::StopDaemonMessage(): Message(STOP_DAEMON, 1024.00) {
		}
		StopDaemonMessage::~StopDaemonMessage() {
		}

		/** RUN_TASK MESSAGE **/
		// TODO: MAke the "1024" below configurable somehow
		RunTaskMessage::RunTaskMessage(std::shared_ptr<WorkflowTask> t, std::string cb): Message(RUN_TASK, 1024.0) {
			this->task = t;
			this->callback_mailbox = cb;
		}
		RunTaskMessage::~RunTaskMessage() {
		}

		/** TASK_DONE MESSAGE **/
		// TODO: MAke the "1024" below configurable somehow
		TaskDoneMessage::TaskDoneMessage(std::shared_ptr<WorkflowTask> t): Message(TASK_DONE, 1024.0) {
			this->task = t;
		}
		TaskDoneMessage::~TaskDoneMessage() {
		}


};
