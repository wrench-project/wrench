/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
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

namespace wrench {

		class MSG_DaemonWithMailbox {

		public:
				std::string mailbox;
				std::string process_name;

				void start(std::string hostname);

		protected:
				MSG_DaemonWithMailbox(std::string mailbox_prefix, std::string process_name);

				virtual ~MSG_DaemonWithMailbox();

		private:
				static int getNewUniqueNumber();


				static int main_stub(int argc, char **argv);  // must be static to be passed to SimGrid
				virtual int main() = 0;


		};
};


#endif //WRENCH_DAEMONWITHMAILBOX_H
