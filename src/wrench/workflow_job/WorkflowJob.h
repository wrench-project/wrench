/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief An abstract Job class
 */


#ifndef WRENCH_WORKFLOWJOB_H
#define WRENCH_WORKFLOWJOB_H


#include <string>
#include <stack>

namespace wrench {

		class Workflow;

		class WorkflowJob {
		public:
				enum Type {
						STANDARD,
						PILOT
				};


				Type getType();
				std::string getTypeAsString();
				std::string getName();


				// Should be excluded from documentation
				std::string popCallbackMailbox();
				void pushCallbackMailbox(std::string);
				std::string getCallbackMailbox();
				std::string getOriginCallbackMailbox();


		protected:

				std::stack<std::string> callback_mailbox_stack;		// Stack of callback mailboxes
				Workflow *workflow;
				Type type;
				std::string name;
				unsigned long getNewUniqueNumber();



		};

};


#endif //WRENCH_WORKFLOWJOB_H
