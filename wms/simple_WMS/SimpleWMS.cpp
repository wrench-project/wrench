//
// Created by Henri Casanova on 2/22/17.
//

#include "SimpleWMS.h"
#include "./SimpleWMSDaemon.h"

namespace WRENCH {


		SimpleWMS::SimpleWMS(Platform *p, Workflow *w, std::string hostname): WMS(p, w) {
			// Create the daemon
			this->wms_process = new SimpleWMSDaemon(p, w);
			this->wms_process->start(hostname);

		}

		SimpleWMS::~SimpleWMS() {

		}


};