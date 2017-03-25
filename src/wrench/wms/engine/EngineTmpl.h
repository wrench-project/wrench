/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_ENGINETMPL_H
#define WRENCH_ENGINETMPL_H

#include <job_manager/JobManager.h>
#include "wms/WMS.h"
#include "wms/engine/EngineDaemon.h"

namespace wrench {

		class Simulation; // forward ref

		// Curiously Recurring Template Pattern (CRTP)
		template<int TYPE, typename IMPL, typename DAEMON>
		class EngineTmpl : public WMS {

				enum {
						_WMS_ID = TYPE
				};

		public:
				static std::unique_ptr<WMS> Create() { return std::unique_ptr<WMS>(new IMPL()); }
				static const uint16_t WMS_ID; // for registration

				/**
				 * @brief Configure the WMS with a workflow instance and a scheduler implementation
				 *
				 * @param s is a pointer to a simulation object
				 * @param w is a pointer to a workflow to execute
				 * @param sc is a pointer to a scheduler implementation
				 * @param hostname
				 */
				void configure(Simulation *s, Workflow *w, Scheduler *sc, std::string hostname) {
					// Create the daemon
					this->wms_process = std::unique_ptr<EngineDaemon>(new DAEMON(s, w, sc));
					// Start the daemon
					this->wms_process->start(hostname);
				}

		protected:
				EngineTmpl() { wms_type = WMS_ID; }

		private:
				Simulation *simulation;
				Workflow *workflow;
				Scheduler *scheduler;
				std::unique_ptr<EngineDaemon> wms_process;
		};


}

#endif //WRENCH_ENGINETMPL_H
