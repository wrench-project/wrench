//
// Created by Henri Casanova on 2/22/17.
//

#ifndef SIMULATION_SIMGRIDMAILBOX_H
#define SIMULATION_SIMGRIDMAILBOX_H

#include <string>
#include "SimgridMessages.h"

namespace WRENCH {

		class SimgridMailbox {

		public:
				SimgridMailbox();
				~SimgridMailbox();

				static SimgridMessage *get(std::string mailbox);
				static void put(std::string mailbox, SimgridMessage *m);

		};

};


#endif //SIMULATION_SIMGRIDMAILBOX_H
