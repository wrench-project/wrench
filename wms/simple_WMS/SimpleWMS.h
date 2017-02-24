//
// Created by Henri Casanova on 2/22/17.
//

#ifndef WRENCH_SIMPLEWMS_H
#define WRENCH_SIMPLEWMS_H

#include "../WMS.h"
#include "SimpleWMSDaemon.h"

namespace WRENCH {

		class Simulation;

		class SimpleWMS : public WMS {


		public:
				SimpleWMS(Simulation *s, Workflow *w, std::string hostname);
				~SimpleWMS();

		private:
				std::unique_ptr<SimpleWMSDaemon> wms_process;
		};
}


#endif //WRENCH_SIMPLEWMS_H
