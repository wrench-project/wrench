/**
 *  @brief WRENCH::Exception implements a simple Exception class.
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
