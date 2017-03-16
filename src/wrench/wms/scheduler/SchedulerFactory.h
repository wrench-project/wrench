/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_SCHEDULERFACTORY_H
#define WRENCH_SCHEDULERFACTORY_H

#include <map>
#include "wms/scheduler/SchedulerTmpl.h"

namespace wrench {

	class SchedulerFactory {

	public:
		typedef Scheduler *(*t_pfFactory)();

		static SchedulerFactory *getInstance();
		uint16_t Register(uint16_t sched_id, t_pfFactory factoryMethod);
		Scheduler *Create(uint16_t sched_id);

		std::map<uint16_t, t_pfFactory> s_list;

	private:
		SchedulerFactory();
	};

	template<int TYPE, typename IMPL>
	const uint16_t SchedulerTmpl<TYPE, IMPL>::SCHED_ID = SchedulerFactory::getInstance()->Register(
			SchedulerTmpl<TYPE, IMPL>::_SCHED_ID, &SchedulerTmpl<TYPE, IMPL>::Create);
}

#endif //WRENCH_SCHEDULERFACTORY_H
