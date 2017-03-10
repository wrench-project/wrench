/**
 *  @file    Message.h
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

#ifndef WRENCH_SIMGRIDMESSAGES_H
#define WRENCH_SIMGRIDMESSAGES_H

#include <string>
#include <compute_services/ComputeService.h>
#include "workflow/WorkflowTask.h"

namespace WRENCH {


		// Base struct
		struct SimulationMessage {

				// Message type enum
				enum Type {
						STOP_DAEMON,
						RUN_TASK,
						TASK_DONE,
				};

				SimulationMessage(Type t, double s);
				virtual ~SimulationMessage();

				Type type;
				double size;
		};

		// Derived struct
		struct StopDaemonMessage: public SimulationMessage {
				StopDaemonMessage();
				~StopDaemonMessage();
		};

		// Derived struct
		struct RunTaskMessage: public SimulationMessage {
				RunTaskMessage(WorkflowTask*, std::string cb);
				~RunTaskMessage();
				WorkflowTask *task;
				std::string callback_mailbox;
		};

		// Derived struct
		struct TaskDoneMessage: public SimulationMessage {
				TaskDoneMessage(WorkflowTask *, ComputeService*);
				~TaskDoneMessage();
				WorkflowTask *task;
				ComputeService *compute_service;
		};

};


#endif //WRENCH_SIMGRIDMESSAGES_H
