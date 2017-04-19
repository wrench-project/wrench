/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SIMPLEWMSDAEMON_H
#define WRENCH_SIMPLEWMSDAEMON_H

#include "wms/engine/EngineDaemon.h"

namespace wrench {

		class Simulation; // forward ref

		/***********************/
		/** \cond INTERNAL    */
		/***********************/

		/**
		 * @brief The daemon class for a SimpleWMS
		 */
		class SimpleWMSDaemon : public EngineDaemon {

		public:
				SimpleWMSDaemon(Simulation *simulation, Workflow *workflow, std::unique_ptr<Scheduler> scheduler);

		private:
				int main();
		};

		/***********************/
		/** \endcond INTERNAL */
		/***********************/
}

#endif //WRENCH_SIMPLEWMSDAEMON_H
