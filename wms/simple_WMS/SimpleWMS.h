//
// Created by Henri Casanova on 2/22/17.
//

#ifndef WRENCH_SIMPLEWMS_H
#define WRENCH_SIMPLEWMS_H

#include "../WMS.h"
//#include "../../simulation/Simulation.h"
#include "SimpleWMSDaemon.h"

namespace WRENCH {

//		class Simulation;

		class SimpleWMS : public WMS {


		public:
//				SimpleWMS(std::shared_ptr<Simulation> s, Workflow *w, std::string hostname);
				SimpleWMS(Workflow *w, std::string hostname);
				~SimpleWMS();

		private:
				SimpleWMSDaemon *wms_process;
		};
}


#endif //WRENCH_SIMPLEWMS_H
