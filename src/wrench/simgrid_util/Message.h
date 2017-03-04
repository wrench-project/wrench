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
#include "workflow/WorkflowTask.h"

namespace WRENCH {


		// Base struct
		struct Message {

				// Message type enum
				enum Type {
						STOP_DAEMON,
						RUN_TASK,
						TASK_DONE
				};

				Message(Type t, double s);
				virtual ~Message();

				Type type;
				double size;
		};

		// Derived struct
		struct StopDaemonMessage: public Message {
				StopDaemonMessage();
				~StopDaemonMessage();
		};

		// Derived struct
		struct RunTaskMessage: public Message {
				RunTaskMessage(std::shared_ptr<WorkflowTask>, std::string cb);
				~RunTaskMessage();
				std::shared_ptr<WorkflowTask> task;
				std::string callback_mailbox;
		};

		// Derived struct
		struct TaskDoneMessage: public Message {
				TaskDoneMessage(std::shared_ptr<WorkflowTask>);
				~TaskDoneMessage();
				std::shared_ptr<WorkflowTask> task;
		};

};



#endif //WRENCH_SIMGRIDMESSAGES_H
