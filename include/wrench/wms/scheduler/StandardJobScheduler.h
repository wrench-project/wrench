/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_STANDARDJOBSCHEDULER_H
#define WRENCH_STANDARDJOBSCHEDULER_H


#include <set>
#include <vector>
#include <map>

namespace wrench {

    class ComputeService;
    class DataMovementManager;
    class JobManager;

    class StandardJobScheduler {

    public:

        virtual void scheduleTasks(const std::set<ComputeService *> &compute_services,
                                   const std::map<std::string, std::vector<WorkflowTask *>> &tasks) = 0;

        void setDataMovementManager(DataMovementManager *data_movement_manager) {
          this->data_movement_manager = data_movement_manager;
        }

        void setJobManager(JobManager *job_manager) {
          this->job_manager = job_manager;
        }

    protected:

        DataMovementManager *data_movement_manager;
        JobManager *job_manager;

    };

};


#endif //WRENCH_STANDARDJOBSCHEDULER_H
