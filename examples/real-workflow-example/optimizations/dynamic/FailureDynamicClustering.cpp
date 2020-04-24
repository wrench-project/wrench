/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <xbt.h>

#include "FailureDynamicClustering.h"

WRENCH_LOG_CATEGORY(failure_dynamic_clustering, "Log category for Simple Dynamic Clustering for Failures");

namespace wrench {

    /**
     * @brief Main optimization procedure: Ungroup failed clustered tasks.
     *
     * @param workflow: a workflow
     */
    void FailureDynamicClustering::process(Workflow *workflow) {

        int count = 0;

        for (auto task_map : workflow->getReadyClusters()) {

            if  (task_map.second.size() <= 1) continue;

            for (auto task : task_map.second) {

                if (task->getClusterID().empty() && task->getFailureCount() > 0) continue;

                WRENCH_INFO("Ungrouping task %s due to failure", task->getID().c_str());
                task->setClusterID(task->getID());
                ++count;
            }
        }

        if (count > 0) {
            WRENCH_INFO("Ungrouped %d tasks", count);
        }
    }
}
