/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_EXCEPTION_H
#define WRENCH_EXCEPTION_H

#include <exception>
#include <string>
#include <workflow_execution_events/WorkflowExecutionFailureCause.h>

namespace wrench {

		/***********************/
		/** \cond DEVELOPER    */
		/***********************/

		/**
		 * @brief An generic exception that is thrown whenever something
		 * unexpected (but simulation-valid) occurrs during the simulated
		 * execution of a WMS
		 */
		class WorkflowExecutionException: public std::exception {

		private:
				std::shared_ptr<WorkflowExecutionFailureCause> cause;

		public:

				/**
				 * @brief Get the exception's error message
				 * @return the exception's message, as a C-style string
				 */
				virtual const char* what() const throw()
				{
					return cause->toString().c_str();
				}

        /**
         * @brief Get the failure cause
         * @return the failure cause
         */
        std::shared_ptr<WorkflowExecutionFailureCause> getCause() {
          return this->cause;
        }


				/***********************/
				/** \cond INTERNAL    */
				/***********************/



				/**
				 * @brief Constructor
				 * @param cause: the cause of the failure
				 */
				WorkflowExecutionException(WorkflowExecutionFailureCause *cause) {
					this->cause = std::shared_ptr<WorkflowExecutionFailureCause>(cause);
				}

				/**
				 * @brief Constructor
				 * @param cause: the cause of the failure
				 */
				WorkflowExecutionException(std::shared_ptr<WorkflowExecutionFailureCause> cause) {
					this->cause = cause;
				}

				/***********************/
				/** \endcond           */
				/***********************/
		};


		/***********************/
		/** \endcond          */
		/***********************/


}


#endif //WRENCH_EXCEPTION_H
