//
// Created by Henri Casanova on 2/22/17.
//

#ifndef SIMULATION_SIMPLEWMS_H
#define SIMULATION_SIMPLEWMS_H

#include "../WMS.h"
#include "SimpleWMSDaemon.h"

namespace WRENCH {

		class SimpleWMS : public WMS {

		public:
				SimpleWMS(Platform *p, Workflow *w, std::string hostname);
				~SimpleWMS();

		private:
				SimpleWMSDaemon *wms_process;

		};

}


#endif //SIMULATION_SIMPLEWMS_H
