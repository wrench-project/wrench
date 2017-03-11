/**
 * @brief S4U_Mailbox is a S4U wrapper
 */

#ifndef WRENCH_S4U_MAILBOX_H
#define WRENCH_S4U_MAILBOX_H


#include <simulation/SimulationMessage.h>

namespace WRENCH {
		class S4U_Mailbox {

		public:
				static std::unique_ptr<SimulationMessage> get(std::string mailbox);
				static void put(std::string mailbox, SimulationMessage *m);
				static void iput(std::string mailbox, SimulationMessage *m);
		};
};


#endif //WRENCH_S4U_MAILBOX_H
