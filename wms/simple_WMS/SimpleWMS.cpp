//
// Created by Henri Casanova on 2/22/17.
//

#include "SimpleWMS.h"

namespace WRENCH {

		SimpleWMS::SimpleWMS(Simulation *s, Workflow *w, std::string hostname): WMS(w) {

			// Create the daemon
			this->wms_process = std::unique_ptr<SimpleWMSDaemon>(new SimpleWMSDaemon(s, w));
			this->wms_process->start(hostname);

		}


		SimpleWMS::~SimpleWMS() {

		}


};