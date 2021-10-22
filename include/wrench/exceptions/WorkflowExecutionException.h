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
#include <memory>
#include <string>
#include <string.h>

#include "wrench/failure_causes/FailureCause.h"

namespace wrench {

		/***********************/
		/** \cond DEVELOPER    */
		/***********************/

		/**
		 * @brief An generic exception that is thrown whenever something
		 * unexpected (but simulation-valid) occurs during the simulated
		 * execution of a WMS
		 */
		class WorkflowExecutionException: public std::exception {

		private:
				std::shared_ptr<FailureCause> cause;

		public:

				/**
				 * @brief Get the exception's error message
				 * @return the exception's message, as a C-style string
				 */
				virtual const char* what() const throw()
				{
          // Without the strdup() below, we get some valgrind warnings...
					return strdup(cause->toString().c_str());
				}

        /**
         * @brief Get the failure cause
         * @return the failure cause
         */
        std::shared_ptr<FailureCause> getCause() {
          return this->cause;
        }


				/***********************/
				/** \cond INTERNAL    */
				/***********************/

				/**
				 * @brief Constructor
				 * @param cause: the cause of the failure
				 */
				WorkflowExecutionException(std::shared_ptr<FailureCause> cause) {
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
