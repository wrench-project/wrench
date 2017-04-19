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

#include "wms/optimizations/static/SimpleTaskClustering.h"

namespace wrench {

	SimpleTaskClustering::SimpleTaskClustering(int max_num) : max_num_(max_num) {}

	void SimpleTaskClustering::process(Workflow *workflow) {

		workflow->DAG_node_map;
		std::cout << "========================= STATIC OPTIMIZATION" << std::endl;
	}
}
