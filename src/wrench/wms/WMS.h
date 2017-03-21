/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief wrench::WMS is a mostly abstract implementation of a WMS
 */

#ifndef WRENCH_WMS_H
#define WRENCH_WMS_H

#include "workflow/Workflow.h"
#include "wms/scheduler/Scheduler.h"

namespace wrench {

	class WMS {

	public:
		virtual ~WMS() {};
		virtual void configure(Simulation *s, Workflow *w, Scheduler *sc, std::string hostname) = 0;

	protected:
		WMS() {};
		uint16_t wms_type;
	};

};


#endif //WRENCH_WMS_H
