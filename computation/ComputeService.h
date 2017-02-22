//
// Created by Henri Casanova on 2/21/17.
//

#ifndef SIMULATION_COMPUTESERVICE_H
#define SIMULATION_COMPUTESERVICE_H

#include <string>
#include <simgrid/msg.h>

#include "../simgrid_util/SimulatedService.h"

namespace WRENCH {

		class ComputeService: public SimulatedService {

		public:
				ComputeService(std::string);
				~ComputeService();

		private:
				int main();

		protected:

		};
};


#endif //SIMULATION_COMPUTESERVICE_H
