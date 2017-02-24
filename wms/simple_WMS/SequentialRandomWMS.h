//
// Created by Henri Casanova on 2/22/17.
//

#ifndef WRENCH_SIMPLEWMS_H
#define WRENCH_SIMPLEWMS_H

#include "../WMS.h"
#include "SequentialRandomWMSDaemon.h"

namespace WRENCH {

		class Simulation;

		class SequentialRandomWMS : public WMS {


		public:
				SequentialRandomWMS(Simulation *s, Workflow *w, std::string hostname);
				~SequentialRandomWMS();

		private:
				std::unique_ptr<SequentialRandomWMSDaemon> wms_process;
		};
}


#endif //WRENCH_SIMPLEWMS_H
