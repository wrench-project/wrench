
/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench-dev.h>

#include "NoopScheduler.h"

void NoopScheduler::scheduleTasks(wrench::JobManager *job_manager,
                                  std::map<std::string, std::vector<wrench::WorkflowTask *>> ready_tasks,
                                  const std::set<wrench::ComputeService *> &compute_services) {
  return;
}