/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_ENGINETMPL_H
#define WRENCH_ENGINETMPL_H

#include "job_manager/JobManager.h"
#include "wms/WMS.h"

namespace wrench {

	class Simulation; // forward ref

		/***********************/
		/** \cond INTERNAL     */
		/***********************/

  /**
	 * @brief WMS engine template
	 *
	 */
	/*
	 * (Curiously Recurring Template Pattern - CRTP)
	 */
	template<const char *TYPE, typename IMPL>
	class EngineTmpl : public WMS, public S4U_DaemonWithMailbox {

	public:
		static std::string _WMS_ID;
		static const std::string WMS_ID; // for registration

		static std::unique_ptr<WMS> Create() { return std::unique_ptr<WMS>(new IMPL()); }

		/**
		 * @brief Configure the WMS engine with a Workflow instance and a Scheduler implementation
		 *
		 * @param simulation: a pointer to a Simulation object
		 * @param workflow: a pointer to a Workflow to execute
		 * @param scheduler:  a pointer to a Scheduler implementation
		 * @param hostname: the name of the host on which to start the WMS engine
		 */
		void configure(Simulation *simulation, Workflow *workflow, std::unique_ptr<Scheduler> scheduler,
		               std::string hostname) {

			this->simulation = simulation;
			this->workflow = workflow;
			this->scheduler = std::move(scheduler);

			// Start the daemon
			this->start(hostname);
		}

	protected:
		EngineTmpl() : S4U_DaemonWithMailbox(TYPE, TYPE) { wms_type = WMS_ID; }

	protected:
		Simulation *simulation;
		Workflow *workflow;
		std::unique_ptr<Scheduler> scheduler;
	};

	template<const char *TYPE, typename IMPL>
	std::string EngineTmpl<TYPE, IMPL>::_WMS_ID = TYPE;

		/***********************/
		/** \cond INTERNAL     */
		/***********************/
}

#endif //WRENCH_ENGINETMPL_H
