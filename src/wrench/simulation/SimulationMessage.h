/**
 *  @brief WRENCH::SimulationMessage and derived classes to encapsulate
 *  control/data messages exchanged by simulated processes
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

				Type type;
				double size;
		};

		// Derived struct
		struct StopDaemonMessage: public SimulationMessage {
				StopDaemonMessage();
		};

		// Derived struct
		struct RunTaskMessage: public SimulationMessage {
				RunTaskMessage(WorkflowTask*, std::string cb);
				WorkflowTask *task;
				std::string callback_mailbox;
		};

		// Derived struct
		struct TaskDoneMessage: public SimulationMessage {
				TaskDoneMessage(WorkflowTask *, ComputeService*);
				WorkflowTask *task;
				ComputeService *compute_service;
		};


};

#endif //WRENCH_SIMGRIDMESSAGES_H
