//
// Created by Henri Casanova on 2/22/17.
//

#ifndef WRENCH_SIMGRIDMESSAGES_H
#define WRENCH_SIMGRIDMESSAGES_H

#include <string>
#include "../workflow/WorkflowTask.h"

namespace WRENCH {


		/* Base struct */
		struct Message {

				/* Message type enum */
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

		struct StopDaemonMessage: public Message {
				StopDaemonMessage();
				~StopDaemonMessage();
		};

		struct RunTaskMessage: public Message {
				RunTaskMessage(std::shared_ptr<WorkflowTask>, std::string cb);
				~RunTaskMessage();

				std::shared_ptr<WorkflowTask> task;
				std::string callback_mailbox;
		};

		struct TaskDoneMessage: public Message {
				TaskDoneMessage(std::shared_ptr<WorkflowTask>);
				~TaskDoneMessage();

				std::shared_ptr<WorkflowTask> task;
		};

};



#endif //WRENCH_SIMGRIDMESSAGES_H
