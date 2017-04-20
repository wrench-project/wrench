/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
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
		 *
		 * @throw std::runtime_error
		 */
		std::string WorkflowJob::getTypeAsString() {
			switch(this->type) {
				case STANDARD: {
					return "Standard";
				}
				case PILOT: {
					return "Pilot";
				}
				default: {
					throw std::runtime_error("WorkflowJob type '" +
																	 std::to_string(this->type) + "' cannot be converted to a string");
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

		/**
		 * @brief Get the number of cores required by the job
		 * @return  the number of cores
		 */
		int WorkflowJob::getNumCores() {
			return this->num_cores;
		}

		/**
		 * @brief Get the duration of the job
		 * @return the duration in seconds
		 */
		double WorkflowJob::getDuration() {
			return this->duration;
		}

		/**
		 * @brief Get the "next" callback mailbox (returns the
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
		 * @brief Get the "origin" callback mailbox
		 * @return the next callback mailbox
		 */
		std::string WorkflowJob::getOriginCallbackMailbox() {
			return this->workflow->getCallbackMailbox();
		}


		/**
		 * @brief Get the "next" callback mailbox (returns the
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
		 * @brief Pushes a callback mailbox
		 * @param mailbox: the mailbox name
		 */
		void WorkflowJob::pushCallbackMailbox(std::string mailbox) {
			this->callback_mailbox_stack.push(mailbox);
		}

		/**
		 * @brief Generate a unique number (for each newly generated job)
		 *
		 * @return a unique number
		 */
		unsigned long WorkflowJob::getNewUniqueNumber() {
			static unsigned long sequence_number = 0;
			return (sequence_number++);
		}

};