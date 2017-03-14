/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief WRENCH::SequentialRandomWMS implements a simple WMS abstraction
 */

#ifndef WRENCH_SIMPLEWMS_H
#define WRENCH_SIMPLEWMS_H

#include "wms/WMS.h"
#include "SequentialRandomWMSDaemon.h"

namespace wrench {

		class Simulation;

		class SequentialRandomWMS : public WMS {

		public:
				SequentialRandomWMS(Simulation *s, Workflow *w, std::string hostname);

		private:
				std::unique_ptr<SequentialRandomWMSDaemon> wms_process;
		};
}


#endif //WRENCH_SIMPLEWMS_H
