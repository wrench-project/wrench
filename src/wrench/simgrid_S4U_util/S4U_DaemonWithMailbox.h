/**
 * @brief S4U_DaemonWithMailbox implements a generic "running daemon that
 *        listens on a mailbox" abstraction
 */

#ifndef WRENCH_SIM4U_DAEMONWITHMAILBOX_H
#define WRENCH_SIM4U_DAEMONWITHMAILBOX_H

#include <string>
#include <simgrid/s4u.hpp>

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
