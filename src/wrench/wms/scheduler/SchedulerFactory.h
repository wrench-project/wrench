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

#ifndef WRENCH_SCHEDULERFACTORY_H
#define WRENCH_SCHEDULERFACTORY_H

#include <map>
#include "wms/scheduler/SchedulerTmpl.h"

namespace wrench {

	class SchedulerFactory {

	public:
		typedef std::unique_ptr<Scheduler> (*t_pfFactory)();

		static SchedulerFactory *getInstance();
		std::string Register(std::string sched_id, t_pfFactory factory_method);
		std::unique_ptr<Scheduler> Create(std::string sched_id);

		std::map<std::string, t_pfFactory> s_list;

	private:
		SchedulerFactory();
	};

	template<const char *TYPE, typename IMPL>
	const std::string SchedulerTmpl<TYPE, IMPL>::SCHED_ID = SchedulerFactory::getInstance()->Register(
			SchedulerTmpl<TYPE, IMPL>::_SCHED_ID, &SchedulerTmpl<TYPE, IMPL>::Create);
}

#endif //WRENCH_SCHEDULERFACTORY_H
