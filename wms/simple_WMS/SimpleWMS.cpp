//
// Created by Henri Casanova on 2/22/17.
//

#include "SimpleWMS.h"

namespace WRENCH {

//		SimpleWMS::SimpleWMS(std::shared_ptr<Simulation> s, Workflow *w, std::string hostname): WMS(w) {
		SimpleWMS::SimpleWMS(Workflow *w, std::string hostname): WMS(w) {
			// Create the daemon
			this->wms_process = new SimpleWMSDaemon(w);
			this->wms_process->start(hostname);

		}


		SimpleWMS::~SimpleWMS() {

		}


};