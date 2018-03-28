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
    class WorkflowTask;

    /**
     * @brief A (virtual) base class for implementing standard job schedulers
     */
    class StandardJobScheduler {

    public:

        /**
         * @brief Method that schedules a set of tasks a standard jobs, according to whatever decision algorithm
         *        it implements, over a set of compute services
         * @param compute_services: the set of compute services
         * @param tasks: the set of tasks
         */
        virtual void scheduleTasks(const std::set<ComputeService *> &compute_services,
                                   const std::map<std::string, std::vector<WorkflowTask *>> &tasks) = 0;

        /**
         * @brief Set a reference to the data manager to be used by this scheduler (nullptr: none is used)
         * @param data_movement_manager: a data movement manager
         */
        void setDataMovementManager(DataMovementManager *data_movement_manager) {
          this->data_movement_manager = data_movement_manager;
        }

        /**
         * @brief Set a reference to the job manager to be used by this scheduler (nullptr: none is used)
         * @param job_manager: a job manager
         */
        void setJobManager(JobManager *job_manager) {
          this->job_manager = job_manager;
        }

    protected:

        /** @brief A data movement manager that the scheduler can use */
        DataMovementManager *data_movement_manager;
        /** @brief A job manager tha the scheduler can use */
        JobManager *job_manager;

    };

};


#endif //WRENCH_STANDARDJOBSCHEDULER_H
