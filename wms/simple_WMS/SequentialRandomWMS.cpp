//
// Created by Henri Casanova on 2/22/17.
//

#include "SequentialRandomWMS.h"

namespace WRENCH {

		SequentialRandomWMS::SequentialRandomWMS(Simulation *s, Workflow *w, std::string hostname): WMS(w) {

			// Create the daemon
			this->wms_process = std::unique_ptr<SequentialRandomWMSDaemon>(new SequentialRandomWMSDaemon(s, w));
			this->wms_process->start(hostname);

		}


		SequentialRandomWMS::~SequentialRandomWMS() {

		}


};