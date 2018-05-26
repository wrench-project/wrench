/**
 * Copyright (c) 2017-2018. The WRENCH Team.
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
     * @brief A (mostly virtual) base class for implementing standard job schedulers
     */
    class StandardJobScheduler {

    public:

        /**
         * @brief Constructor
         */
        StandardJobScheduler() {
          this->data_movement_manager = nullptr;
          this->job_manager = nullptr;
        }

        /**
         * @brief Schedules a set of tasks a standard jobs, according to whatever decision algorithm
         *        it implements, over a set of compute services
         * @param compute_services: the set of compute services
         * @param tasks: the set of tasks
         */
        virtual void scheduleTasks(const std::set<ComputeService *> &compute_services,
                                   const std::vector<WorkflowTask *> &tasks) = 0;

#if 0
//        /**
//         * @brief Schedules a set of clustered tasks a standard jobs, according to whatever decision
//         *        algorithm it implements, over a set of compute services
//         * @param compute_services: the set of compute services
//         * @param tasks: the map of clustered tasks
//         */
//        virtual void scheduleTasks(const std::set<ComputeService *> &compute_services,
//                                   const std::map<std::string, std::vector<WorkflowTask *>> &tasks) {
//          throw std::runtime_error("StandardJobScheduler::scheduleTasks(): Not implemented yet!");
//        }
#endif

        /**
         * @brief Set a reference to the data manager to be used by this scheduler (nullptr: none is used)
         * @param data_movement_manager: a data movement manager
         */
        void setDataMovementManager(DataMovementManager *data_movement_manager) {
          this->data_movement_manager = data_movement_manager;
        }

        /**
         * @brief Get a reference to the data movement manager to be used by this scheduler (nullptr: none is used)
         * @return a data movement manager
         */
        DataMovementManager *getDataMovementManager() {
          return this->data_movement_manager;
        }

        /**
         * @brief Set a reference to the job manager to be used by this scheduler (nullptr: none is used)
         * @param job_manager: a job manager
         */
        void setJobManager(JobManager *job_manager) {
          this->job_manager = job_manager;
        }

        /**
         * @brief Get a reference to the job manager to be used by this scheduler (nullptr: none is used)
         * @return a job manager
         */
        JobManager *getJobManager() {
          return this->job_manager;
        }

    private:
        DataMovementManager *data_movement_manager;
        JobManager *job_manager;

    };

};


#endif //WRENCH_STANDARDJOBSCHEDULER_H
