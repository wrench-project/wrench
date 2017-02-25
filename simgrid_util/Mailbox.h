/**
 *  @file    Mailbox.h
 *  @author  Henri Casanova
 *  @date    2/21/2017
 *  @version 1.0
 *
 *  @brief WRENCH::Mailbox class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::Mailbox class is a MSG wrapper
 *
 */

#ifndef WRENCH_SIMGRIDMAILBOX_H
#define WRENCH_SIMGRIDMAILBOX_H

#include <string>
#include "Message.h"

namespace WRENCH {

		class Mailbox {
		public:
				static std::unique_ptr<Message> get(std::string mailbox);
				static void put(std::string mailbox, Message *m);
				static void iput(std::string mailbox, Message *m);

		};

};


#endif //WRENCH_SIMGRIDMAILBOX_H
