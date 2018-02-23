/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <xbt.h>

#include "wrench/logging/TerminalOutput.h"
#include "wrench/wms/scheduler/Scheduler.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(scheduler, "Log category for Scheduler");

namespace wrench {

    /**
     * @brief Get the total number of flops for a list of tasks
     *
     * @param tasks: list of tasks
     *
     * @return the total number of flops
     */
    double Scheduler::getTotalFlops(std::vector<WorkflowTask *> tasks) {
      double total_flops = 0;
      for (auto it : tasks) {
        total_flops += (*it).getFlops();
      }
      return total_flops;
    }
}
