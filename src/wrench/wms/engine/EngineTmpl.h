/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief wrench::EngineTmpl
 */

#ifndef WRENCH_ENGINETMPL_H
#define WRENCH_ENGINETMPL_H

#include <job_manager/JobManager.h>
#include "wms/WMS.h"
#include "wms/engine/EngineDaemon.h"

namespace wrench {

	class Simulation; // forward ref

	// Curiously Recurring Template Pattern (CRTP)
	template<const char *TYPE, typename IMPL, typename DAEMON>
	class EngineTmpl : public WMS {

	public:
		static std::string _WMS_ID;
		static const std::string WMS_ID; // for registration

		static std::unique_ptr<WMS> Create() { return std::unique_ptr<WMS>(new IMPL()); }

		/**
		 * @brief Configure the WMS with a workflow instance and a scheduler implementation
		 *
		 * @param simulation is a pointer to a simulation object
		 * @param workflow is a pointer to a workflow to execute
		 * @param scheduler is a pointer to a scheduler implementation
		 * @param hostname
		 */
		void configure(Simulation *simulation,
		               Workflow *workflow,
		               std::unique_ptr<Scheduler> scheduler,
		               std::string hostname) {

			// Create the daemon
			this->wms_process = std::unique_ptr<EngineDaemon>(new DAEMON(simulation, workflow, std::move(scheduler)));
			// Start the daemon
			this->wms_process->start(hostname);
		}

	protected:
		EngineTmpl() { wms_type = WMS_ID; }

	private:
		Simulation *simulation;
		Workflow *workflow;
		std::unique_ptr<Scheduler> scheduler;
		std::unique_ptr<EngineDaemon> wms_process;
	};

	template<const char *TYPE, typename IMPL, typename DAEMON>
	std::string EngineTmpl<TYPE, IMPL, DAEMON>::_WMS_ID = TYPE;
}

#endif //WRENCH_ENGINETMPL_H
