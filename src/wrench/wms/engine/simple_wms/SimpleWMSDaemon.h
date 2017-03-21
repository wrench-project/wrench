/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::SimpleWMSDaemon implements the daemon for a simple WMS abstraction
 */

#ifndef WRENCH_SIMPLEWMSDAEMON_H
#define WRENCH_SIMPLEWMSDAEMON_H

#include "wms/engine/EngineDaemon.h"

namespace wrench {

	class Simulation; // forward ref

	class SimpleWMSDaemon : public EngineDaemon {

	public:
		SimpleWMSDaemon(Simulation *, Workflow *w, Scheduler *s);

	private:
		int main();
	};
}

#endif //WRENCH_SIMPLEWMSDAEMON_H
