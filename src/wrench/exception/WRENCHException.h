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

		/**
		 * @brief A simple Exception class
		 */
		class WRENCHException: public std::exception {

		private:
				std::string message;

		public:

				/**
				 * @brief Constructor
				 * @param string: the exception's string message
				 */
				WRENCHException(const std::string string) {
					message = string;
				}

				virtual const char* what() const throw()
				{
					return message.c_str();
				}

		};

}


#endif //WRENCH_EXCEPTION_H
