//
// Created by Henri Casanova on 3/9/17.
//

#ifndef WRENCH_SIM4U_DAEMONWITHMAILBOX_H
#define WRENCH_SIM4U_DAEMONWITHMAILBOX_H

#include <string>
#include <simgrid/s4u.hpp>
//#include "Sim4U_DaemonWithMailboxActor.h"

namespace WRENCH {

		class S4U_DaemonWithMailbox {

		public:
				std::string process_name;
				std::string mailbox_name;

				S4U_DaemonWithMailbox(std::string process_name, std::string mailbox_prefix);
				void start(std::string hostname);
				virtual int main() = 0;

		private:

				simgrid::s4u::ActorPtr actor;

		};
};


#endif //WRENCH_SIM4U_DAEMONWITHMAILBOX_H
