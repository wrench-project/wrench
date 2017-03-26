/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief wrench::SchedulerFactory is a factory class for schedulers.
 */

#include "wms/scheduler/SchedulerFactory.h"

namespace wrench {

	SchedulerFactory *SchedulerFactory::getInstance() {
		static SchedulerFactory fact;
		return &fact;
	}

	std::string SchedulerFactory::Register(std::string sched_id, t_pfFactory factory_method) {
		s_list[sched_id] = factory_method;
		return sched_id;
	}

	std::unique_ptr<Scheduler> SchedulerFactory::Create(std::string sched_id) {
		return s_list[sched_id]();
	}

	SchedulerFactory::SchedulerFactory() {}
}