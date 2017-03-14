/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::Exception implements a simple Exception class.
 */

#ifndef WRENCH_EXCEPTION_H
#define WRENCH_EXCEPTION_H

#include <exception>
#include <string>

namespace wrench {

		class WRENCHException: public std::exception {

		private:
				std::string message;

		public:

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
