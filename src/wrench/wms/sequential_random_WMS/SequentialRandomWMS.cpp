/**
 * @brief WRENCH::SequentialRandomWMS implements a simple WMS abstraction
 */

#include "SequentialRandomWMS.h"

namespace WRENCH {

		/**
		 * @brief Constructor
		 *
		 * @param s is a pointer to a simulation
		 * @param w is a pointer to a workflow
		 * @param hostname is the hostname on which the WMS daemon runs
		 */
		SequentialRandomWMS::SequentialRandomWMS(Simulation *s, Workflow *w, std::string hostname): WMS(w) {
			// Create the daemon
			this->wms_process = std::unique_ptr<SequentialRandomWMSDaemon>(new SequentialRandomWMSDaemon(s, w));
			// Start the daemon
			this->wms_process->start(hostname);
		}

};