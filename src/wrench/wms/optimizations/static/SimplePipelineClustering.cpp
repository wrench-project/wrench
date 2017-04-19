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

#include "wms/optimizations/static/SimplePipelineClustering.h"
#include <set>

namespace wrench {

	void SimplePipelineClustering::process(Workflow *workflow) {

		std::set<std::string> pipelined_tasks;
		std::set<WorkflowTask *> clustered_tasks;

		for (auto task : workflow->getTasks()) {
			if (task->getNumberOfChildren() == 1 && task->getNumberOfParents() == 1 &&
			    pipelined_tasks.find(task->getId()) == pipelined_tasks.end()) {

				int task_count = 1;
				std::string task_id = "CLUSTER_" + task->getId();
				double task_flops = task->getFlops();
				int task_num_procs = task->getNumProcs();

				// explore parent
				WorkflowTask *parent = workflow->getTaskParents(task)[0];
				while (true) {
					if (parent->getNumberOfChildren() == 1 && parent->getNumberOfParents() == 1 &&
					    pipelined_tasks.find(parent->getId()) == pipelined_tasks.end()) {

						pipelined_tasks.insert(parent->getId());
						clustered_tasks.insert(parent);
						++task_count;
						task_id += "_" + parent->getId();
						task_flops += parent->getFlops();
						task_num_procs = std::max(task_num_procs, parent->getNumProcs());

						// next parent
						parent = workflow->getTaskParents(parent)[0];

					} else {
						break;
					}
				}

				// explore child
				WorkflowTask *child = workflow->getTaskChildren(task)[0];
				while (true) {
					if (child->getNumberOfChildren() == 1 && child->getNumberOfParents() == 1 &&
					    pipelined_tasks.find(child->getId()) == pipelined_tasks.end()) {

						pipelined_tasks.insert(child->getId());
						clustered_tasks.insert(child);
						++task_count;
						task_id += "_" + child->getId();
						task_flops += child->getFlops();
						task_num_procs = std::max(task_num_procs, child->getNumProcs());

						// next child
						child = workflow->getTaskChildren(child)[0];

					} else {
						break;
					}
				}

				// create clustered task
				if (task_count > 1) {
					pipelined_tasks.insert(task->getId());
					clustered_tasks.insert(task);

					WorkflowTask *clustered_task = workflow->addTask(task_id, task_flops, task_num_procs);
					workflow->addControlDependency(parent, clustered_task);
					workflow->addControlDependency(clustered_task, child);
				}
			}
		}
		// remove clustered tasks
		for (auto t : clustered_tasks) {
			workflow->removeTask(t);
		}
	}
}
