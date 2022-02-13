/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_BATCHJOBSETCORELEVEL_H
#define WRENCH_BATCHJOBSETCORELEVEL_H

#include "wrench/services/compute/batch/BatchJob.h"


namespace wrench {

    /***********************/
    /** \cond              */
    /***********************/

    /**
     * @brief A class that implements a batch_standard_and_pilot_jobs job set abstrsaction
     */
    class BatchJobSetCoreLevel {

    public:

        BatchJobSetCoreLevel() = default;

        /** @brief The job set **/
        std::set<std::shared_ptr<BatchJob>> jobs;

        /** @brief Core utilization **/
        std::map<int, unsigned long> core_utilization;

        /**
         * @brief Overloaded += operator
         * @param right: right-hand side
         * @return
         */
        BatchJobSetCoreLevel& operator += (const BatchJobSetCoreLevel& right)
        {
            for (const auto &j : right.jobs) {
                add(j);
            }
            return *this;
        }

        /**
         * @brief Overloaded -= operator
         * @param right: right-hand side
         * @return
         */
        BatchJobSetCoreLevel& operator -= (const BatchJobSetCoreLevel& right)
        {
            for (const auto &j : right.jobs) {
                remove(j);
            }
            return *this;
        }

        /**
         * @brief Add a batch_standard_and_pilot_jobs job to the set
         * @param job: the batch_standard_and_pilot_jobs job
         */
        void inline add(std::shared_ptr<BatchJob> job) {
            if (this->jobs.find(job) == this->jobs.end()) {
                for (auto const &i : job->getAllocatedNodeIndices()) {
                    if (this->core_utilization.find(i) == this->core_utilization.end())  {
                        this->core_utilization[i] = job->getRequestedCoresPerNode();
                    } else {
                        this->core_utilization[i] += job->getRequestedCoresPerNode();
                    }
                }
                this->jobs.insert(job);
            }
        }

        /**
         * @brief Remove a batch_standard_and_pilot_jobs job from the set
         * @param job: the batch_standard_and_pilot_jobs job
         */
        void inline remove(std::shared_ptr<BatchJob> job) {
            if  (this->jobs.find(job) != this->jobs.end()) {
                for (auto const &i : job->getAllocatedNodeIndices()) {
                    this->core_utilization[i] -= job->getRequestedCoresPerNode();
                }
                this->jobs.erase(job);
            }
        }

    };

    /***********************/
    /** \endcond           */
    /***********************/

}

#endif //WRENCH_BATCHJOBSETCORELEVEL_H
