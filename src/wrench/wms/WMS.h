/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_WMS_H
#define WRENCH_WMS_H

#include "workflow/Workflow.h"
#include "wms/scheduler/Scheduler.h"
#include "wms/optimizations/static/StaticOptimization.h"

namespace wrench {

		/**
		 * @brief Abstract implementation of a Workflow Management System
		 */
	class WMS {

	protected:
		WMS() {};
		std::string wms_type;

		Simulation *simulation;
		Workflow *workflow;
		std::unique_ptr<Scheduler> scheduler;
		std::vector<std::unique_ptr<StaticOptimization>> static_optimizations;

	public:
		virtual ~WMS() {};

		virtual void configure(Simulation *simulation,
		                       Workflow *workflow,
		                       std::unique_ptr<Scheduler> scheduler,
		                       std::string hostname) = 0;

		virtual void add_static_optimization(std::unique_ptr<StaticOptimization> optimization) = 0;
	};
};


#endif //WRENCH_WMS_H
