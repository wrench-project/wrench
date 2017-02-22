//
// Created by Henri Casanova on 2/21/17.
//

#ifndef SIMULATION_COMPUTESERVICE_H
#define SIMULATION_COMPUTESERVICE_H

#include <string>
#include <simgrid/msg.h>

namespace WRENCH {

		class ComputeService {

		private:
				std::string service_name;
				std::string mailbox_name; // The SimGrid mailbox the service is listening on

		public:
				ComputeService(std::string);
				~ComputeService();

				void start(std::string);
				void stop();

		private:
				 static int main(int argc, char **argv);

		};
};


#endif //SIMULATION_COMPUTESERVICE_H
