/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief Factory class for schedulers.
 */

#include "wms/scheduler/SchedulerFactory.h"

namespace wrench {

	SchedulerFactory *SchedulerFactory::getInstance() {
		static SchedulerFactory fact;
		return &fact;
	}

	uint16_t SchedulerFactory::Register(uint16_t sched_id, t_pfFactory factoryMethod) {
		s_list[sched_id] = factoryMethod;
		return sched_id;
	}

	Scheduler *SchedulerFactory::Create(uint16_t sched_id) {
		return s_list[sched_id]();
	}

	SchedulerFactory::SchedulerFactory() {}
}