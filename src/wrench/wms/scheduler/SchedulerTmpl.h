/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_SCHEDULERTMPL_H
#define WRENCH_SCHEDULERTMPL_H

#include "wms/scheduler/Scheduler.h"

namespace wrench {

	// Curiously Recurring Template Pattern (CRTP)
	template<int TYPE, typename IMPL>
	class SchedulerTmpl : public Scheduler {

		enum {
			_SCHED_ID = TYPE
		};

	public:
		static Scheduler *Create() { return new IMPL(); }
		static const uint16_t SCHED_ID; // for registration

	protected:
		SchedulerTmpl() { sched_type = SCHED_ID; }
	};
}

#endif //WRENCH_SCHEDULERTMPL_H
