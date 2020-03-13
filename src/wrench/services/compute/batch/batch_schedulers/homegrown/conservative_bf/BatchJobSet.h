/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_BATCHJOBSET_H
#define WRENCH_BATCHJOBSET_H

#include <wrench/services/compute/batch/BatchJob.h>

namespace wrench {

    class BatchJobSet {

    public:

        BatchJobSet() = default;

        std::set<BatchJob *> jobs;
        unsigned long num_nodes_utilized = 0;

        /**
         * @brief Overloaded += operator
         * @param right: right-hand side
         * @return
         */
        BatchJobSet& operator += (const BatchJobSet& right)
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
        BatchJobSet& operator -= (const BatchJobSet& right)
        {
            for (const auto &j : right.jobs) {
                remove(j);
            }
            return *this;
        }

        /**
         * @brief Add a batch job to the set
         * @param job: the batch job
         */
        void inline add(BatchJob *job) {
            if (this->jobs.find(job) == this->jobs.end()) {
                num_nodes_utilized += job->getRequestedNumNodes();
                this->jobs.insert(job);
            }
        }

        /**
         * @brief Remove a batch job from the set
         * @param job: the batch job
         */
        void inline remove(BatchJob *job) {
            if  (this->jobs.find(job) != this->jobs.end()) {
                num_nodes_utilized -= job->getRequestedNumNodes();
                this->jobs.erase(job);
            }
        }

    };

}

#endif //WRENCH_BATCHJOBSET_H
