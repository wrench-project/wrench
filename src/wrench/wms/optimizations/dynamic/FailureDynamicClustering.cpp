/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <xbt.h>

#include "logging/TerminalOutput.h"
#include "wms/optimizations/dynamic/FailureDynamicClustering.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(failure_dynamic_clustering, "Log category for Simple Dynamic Clustering for Failures");

namespace wrench {

    /**
     * @brief Main function for dynamic optimizations: ungrouping failed clustered tasks.
     *
     * @param workflow: a pointer to a Workflow object
     */
    void FailureDynamicClustering::process(Workflow *workflow) {

      int count = 0;

      for (auto task_map : workflow->getReadyTasks()) {
        if (task_map.second.size() > 1) {
          for (auto task : task_map.second) {
            if (not task->getClusterId().empty() && task->getFailureCount() > 0) {
              WRENCH_INFO("Ungrouping task %s due to failure", task->getId().c_str());
              task->setClusterId(task->getId());
              ++count;
            }
          }
        }
      }
      unsigned long final_size = workflow->getReadyTasks().size();

      if (count > 0) {
        WRENCH_INFO("Ungrouped %d tasks", count);
      }
    }
}