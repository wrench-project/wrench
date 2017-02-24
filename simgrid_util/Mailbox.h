//
// Created by Henri Casanova on 2/22/17.
//

#ifndef WRENCH_SIMGRIDMAILBOX_H
#define WRENCH_SIMGRIDMAILBOX_H

#include <string>
#include "Message.h"

namespace WRENCH {

		class Mailbox {

		public:
				Mailbox();
				~Mailbox();

				static std::unique_ptr<Message> get(std::string mailbox);
				static void put(std::string mailbox, Message *m);

		};

};


#endif //WRENCH_SIMGRIDMAILBOX_H
