/**
 *  @file    DaemonWithMailbox.h
 *  @author  Henri Casanova
 *  @date    2/24/2017
 *  @version 1.0
 *
 *  @brief WRENCH::DaemonWithMailbox class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::DaemonWithMailbox class is a MSG wrapper for a running processes that listens on a particular mailbox
 *
 */


#ifndef WRENCH_DAEMONWITHMAILBOX_H
#define WRENCH_DAEMONWITHMAILBOX_H

#include <string>

namespace WRENCH {

		class DaemonWithMailbox {

		public:
				std::string mailbox;
				std::string process_name;

				void start(std::string hostname);

		protected:
				DaemonWithMailbox(std::string mailbox_prefix, std::string process_name);

				virtual ~DaemonWithMailbox();

		private:
				static int getNewUniqueNumber();


				static int main_stub(int argc, char **argv);  // must be static to be passed to SimGrid
				virtual int main() = 0;


		};
};


#endif //WRENCH_DAEMONWITHMAILBOX_H
