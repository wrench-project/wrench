#ifndef WRENCH_SIMGRIDMAILBOX_H
#define WRENCH_SIMGRIDMAILBOX_H

#include <string>
#include "simulation/SimulationMessage.h"

namespace WRENCH {

		class MSG_Mailbox {
		public:
				static std::unique_ptr<SimulationMessage> get(std::string mailbox);
				static void put(std::string mailbox, SimulationMessage *m);
				static void iput(std::string mailbox, SimulationMessage *m);
				static void iput_failure_handler(void*);

		};

};


#endif //WRENCH_SIMGRIDMAILBOX_H
