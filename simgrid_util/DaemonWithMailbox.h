//
// Created by Henri Casanova on 2/21/17.
//

#ifndef SIMULATION_SIMULATEDSERVICE_H
#define SIMULATION_SIMULATEDSERVICE_H

#include <string>

namespace WRENCH {

		class DaemonWithMailbox {

		public:
				std::string mailbox;

				void start(std::string hostname);

		protected:
				DaemonWithMailbox(std::string mailbox_prefix);
				virtual ~DaemonWithMailbox();

		private:
				static int main_stub(int argc, char **argv);  // must be static
				virtual int main() = 0; // purely virtual
				static int getNewUniqueNumber();



		};
};


#endif //SIMULATION_SIMULATEDSERVICE_H
