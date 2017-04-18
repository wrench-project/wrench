/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SCHEDULERTMPL_H
#define WRENCH_SCHEDULERTMPL_H

#include "wms/scheduler/Scheduler.h"

namespace wrench {

		/**
		 * @brief A Scheduler template
		 */
		/*
		 * (Curiously Recurring Template Pattern - CRTP)
		 */
		template<const char *TYPE, typename IMPL>
		class SchedulerTmpl : public Scheduler {

		public:
				static std::string _SCHED_ID;
				static std::unique_ptr<Scheduler> Create() { return std::unique_ptr<Scheduler>(new IMPL()); }
				static const std::string SCHED_ID; // for registration

		protected:
				SchedulerTmpl() { sched_type = SCHED_ID; }
		};

		template<const char *TYPE, typename IMPL>
		std::string SchedulerTmpl<TYPE, IMPL>::_SCHED_ID = TYPE;

}

#endif //WRENCH_SCHEDULERTMPL_H
