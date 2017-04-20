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

namespace wrench {

		/***********************/
		/** \cond DEVELOPER    */
		/***********************/

		/**
		 * @brief An exception that is thrown when a process attempts
		 * to interact with a service that is in the DOWN state
		 */
		class ServiceIsDownException: public std::exception {

		private:
				std::string error_message;

		public:

				/**
				 * @brief Get the exception's error message
				 * @return the exception's message, as a C-style string
				 */
				virtual const char* what() const throw()
				{
					return error_message.c_str();
				}

				/***********************/
				/** \cond INTERNAL    */
				/***********************/

				/**
				 * @brief Constructor
				 * @param service_name: the service name
				 */
				ServiceIsDownException(const std::string service_name) {
					this->error_message = "Service '"+ service_name + "' is down";
				}

				/***********************/
				/** \endcond INTERNAL  */
				/***********************/
		};


		/***********************/
		/** \endcond DEVELOPER */
		/***********************/


}


#endif //WRENCH_EXCEPTION_H
