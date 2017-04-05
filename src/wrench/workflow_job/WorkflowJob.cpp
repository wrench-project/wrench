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


#include <string>
#include <simgrid_S4U_util/S4U_Mailbox.h>

#include "workflow_job/WorkflowJob.h"
#include "workflow/Workflow.h"

namespace wrench {

		/**
		 * @brief Get the job type
		 * @return the type
		 */
		WorkflowJob::Type WorkflowJob::getType() {
			return this->type;
		}

		/**
		 * @brief Get the job type name
		 * @return the name of the type
		 */
		std::string WorkflowJob::getTypeAsString() {
			switch(this->type) {
				case STANDARD: {
					return "Standard";
				}
				case PILOT: {
					return "Pilot";
				}
			}
		}

		/**
		 * @brief Get the job's name
		 * @return the name as a string
		 */
		std::string WorkflowJob::getName() {
			return this->name;
		}

		/***********************************************************/
		/**	UNDOCUMENTED PUBLIC/PRIVATE  METHODS AFTER THIS POINT **/
		/***********************************************************/

		/*! \cond PRIVATE */

		/**
		 * @brief Gets the "next" callback mailbox (returns the
		 *         origin (i.e., workflow) mailbox if the mailbox stack is empty)
		 * @return the next callback mailbox
		 */
		std::string WorkflowJob::getCallbackMailbox() {
			if (this->callback_mailbox_stack.size() == 0) {
				return this->workflow->getCallbackMailbox();
			} else {
				return this->callback_mailbox_stack.top();
			}
		}

		/**
		 * @brief Gets the "origin" callback mailbox
		 * @return the next callback mailbox
		 */
		std::string WorkflowJob::getOriginCallbackMailbox() {
				return this->workflow->getCallbackMailbox();
		}


		/**
		 * @brief Gets the "next" callback mailbox (returns the
		 *         workflow mailbox if the mailbox stack is empty), and
		 *         pops it
		 * @return the next callback mailbox
		 */
		std::string WorkflowJob::popCallbackMailbox() {
			if (this->callback_mailbox_stack.size() == 0) {
				return this->workflow->getCallbackMailbox();
			} else {
				std::string mailbox = this->callback_mailbox_stack.top();
				this->callback_mailbox_stack.pop();
				return mailbox;
			}
		}

		/**
		 * @brief pushes a callback mailbox
		 * @param mailbox is the mailbox name
		 */
		void WorkflowJob::pushCallbackMailbox(std::string mailbox) {
			this->callback_mailbox_stack.push(mailbox);
		}


		/**
		 * @brief This method is used to generate a unique number for each newly generated job,
		 *
		 * @return a unique number
		 */
		unsigned long WorkflowJob::getNewUniqueNumber() {
			static unsigned long sequence_number = 0;
			return (sequence_number++);
		}

		/*! \endcond */
};