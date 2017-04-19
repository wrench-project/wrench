/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief wrench::StaticSimpleClustering implements a simple task
 *  clustering based on the max number of tasks per cluster
 */

#ifndef WRENCH_SIMPLETASKCLUSTERING_H
#define WRENCH_SIMPLETASKCLUSTERING_H

#include "wms/optimizations/static/StaticOptimization.h"

namespace wrench {

	class SimpleTaskClustering : public StaticOptimization {
	public:
		SimpleTaskClustering(int max_num);

		void process(Workflow *workflow);

	private:
		int max_num_;
	};
}

#endif //WRENCH_SIMPLETASKCLUSTERING_H
