//
// Created by Henri Casanova on 2/21/17.
//

#ifndef SIMULATION_SIMULATEDSERVICE_H
#define SIMULATION_SIMULATEDSERVICE_H

#include <string>

namespace WRENCH {

		class SimulatedService {

		public:
				void start(std::string hostname);

		protected:
				static int main_stub(int argc, char **argv);  // must be static
				virtual int main() = 0; // purely virtual
				static int getNewUniqueNumber();

				std::string service_name;
				std::string mailbox_name;

		};
};


#endif //SIMULATION_SIMULATEDSERVICE_H
