/**
 *  @file    Exception.h
 *  @author  Henri Casanova
 *  @date    2/21/2017
 *  @version 1.0
 *
 *  @brief WRENCH::Exception class implementation
 *
 *  @section DESCRIPTION
 *
 *  For now, just a basic Exception class
 *
 */

#ifndef WRENCH_EXCEPTION_H
#define WRENCH_EXCEPTION_H

#include <exception>
#include <string>

namespace WRENCH {

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
