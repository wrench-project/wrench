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


    class BatchJob;

    class BatchJobSet {

    public:

        BatchJobSet() = default;

        std::set<BatchJob *> jobs;
        unsigned long num_nodes_utilized = 0;

        BatchJobSet& operator += (const BatchJobSet& right)
        { jobs.insert(right.jobs.begin(), right.jobs.end());
          num_nodes_utilized += right.num_nodes_utilized;
          return *this;
        }

        BatchJobSet& operator -= (const BatchJobSet& right)
        {
            for (const auto &j : right.jobs) {
                if (jobs.find(j) != jobs.end()) {
                    num_nodes_utilized -=  j->getRequestedNumNodes();
                    jobs.erase(j);
                }
            }
//            if (jobs.find())
//            jobs.erase(right.jobs.begin(), right.jobs.end());
//            num_nodes_utilized -= right.num_nodes_utilized;
            return *this;
        }


        void add(BatchJob *job) {
            num_nodes_utilized += job->getRequestedNumNodes();
            this->jobs.insert(job);
        }

        void remove(BatchJob *job) {
            num_nodes_utilized -= job->getRequestedNumNodes();
            this->jobs.erase(job);
        }


    };



}


#endif //WRENCH_BATCHJOBSET_H
