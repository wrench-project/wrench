//
// Created by Henri Casanova on 2/19/17.
//

#ifndef WRENCH_EXCEPTION_H
#define WRENCH_EXCEPTION_H

#include <exception>
#include <string>

namespace WRENCH {

		class Exception: public std::exception {

		private:
				std::string message;

		public:

				Exception(const std::string string) {
					message = string;
				}

				virtual const char* what() const throw()
				{
					return message.c_str();
				}

		};

}


#endif //WRENCH_EXCEPTION_H
