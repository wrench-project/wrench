/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_BATCHSCHEDULER_H
#define WRENCH_BATCHSCHEDULER_H

#include <deque>
#include "wrench/services/compute/batch/BatchJob.h"

/***********************/
/** \cond INTERNAL    **/
/***********************/

namespace wrench {

    class BatchComputeService;

    class BatchScheduler {

    public:

        virtual ~BatchScheduler() = default;

        explicit BatchScheduler(BatchComputeService *cs) : cs(cs) {};

        /**
         * Virtual methods to override
         */

        virtual void init() = 0;

        virtual void launch() = 0;

        virtual void shutdown() = 0;

        virtual void processQueuedJobs() = 0;

        virtual void processJobSubmission(std::shared_ptr<BatchJob> batch_job) = 0;
        virtual void processJobFailure(std::shared_ptr<BatchJob> batch_job) = 0;
        virtual void processJobCompletion(std::shared_ptr<BatchJob> batch_job) = 0;
        virtual void processJobTermination(std::shared_ptr<BatchJob> batch_job) = 0;


        virtual std::map<std::string, double> getStartTimeEstimates(std::set<std::tuple<std::string, unsigned long, unsigned long, double>> set_of_jobs) = 0;



    protected:
        // Batch Compute service this scheduler is for
        BatchComputeService *cs;

    };
}

/***********************/
/** \endcond          **/
/***********************/


#endif //WRENCH_BATCHSCHEDULER_H
