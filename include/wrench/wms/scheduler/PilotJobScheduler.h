/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_PILOTJOBSCHEDULER_H
#define WRENCH_PILOTJOBSCHEDULER_H


#include <set>

namespace wrench {

    class ComputeService;
    class DataMovementManager;
    class JobManager;

    /**
     * @brief A (mostly virtual) base class for implementing PilotJob scheduling algorithms to be used by a WMS
     */
    class PilotJobScheduler {

    public:

        /**
         * @brief Constructor
         */
        PilotJobScheduler() {
          this->data_movement_manager = nullptr;
          this->job_manager = nullptr;
        }

        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        virtual ~PilotJobScheduler() = default;
        /***********************/
        /** \endcond           */
        /***********************/


        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        /**
         * @brief A method that schedules pilot jobs, according to whatever decision algorithm
         *        it implements, over a set of compute services
         * @param compute_services: the set of compute services
         */
        virtual void schedulePilotJobs(const std::set<ComputeService *> &compute_services) = 0;

        /**
         * @brief Get the data movement manager to be used by this scheduler (nullptr: none is used)
         * @return a data movement manager
         */
        DataMovementManager *getDataMovementManager() {
          return this->data_movement_manager;
        }

        /**
         * @brief Get the job manager to be used by this scheduler (nullptr: none is used)
         * @return a job manager
         */
        JobManager *getJobManager() {
          return this->job_manager;
        }

        /***********************/
        /** \endcond           */
        /***********************/

        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        /**
         * @brief Set the data movement manager to be used by this scheduler (nullptr: none is used)
         * @param data_movement_manager: a data movement manager
         */
        void setDataMovementManager(DataMovementManager *data_movement_manager) {
          this->data_movement_manager = data_movement_manager;
        }

        /**
         * @brief Set the job manager to be used by this scheduler (nullptr: none is used)
         * @param job_manager: a job manager
         */
        void setJobManager(JobManager *job_manager) {
          this->job_manager = job_manager;
        }

        /***********************/
        /** \endcond           */
        /***********************/

    private:

        DataMovementManager *data_movement_manager;
        JobManager *job_manager;

    };
};


#endif //WRENCH_PILOTJOBSCHEDULER_H
