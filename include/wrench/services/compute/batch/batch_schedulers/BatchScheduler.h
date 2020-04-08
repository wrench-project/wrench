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

namespace wrench {

/***********************/
/** \cond INTERNAL    **/
/***********************/

    class BatchComputeService;

    /**
     * @brief An abstract class that defines a batch scheduler
     */
    class BatchScheduler {

    public:

        /**
         * @brief Destructor
         */
        virtual ~BatchScheduler() = default;

        /**
         * @brief Constructor
         */
        explicit BatchScheduler(BatchComputeService *cs) : cs(cs) {};

        /**
         * Virtual methods to override
         */

        /**
         * @brief Initialization method
         */
        virtual void init() = 0;

        /**
         * @brief Launch method
         */
        virtual void launch() = 0;

        /**
         * @brief Shutdown method
         */
        virtual void shutdown() = 0;

        /**
         * @brief Method to process queued jobs
         */
        virtual void processQueuedJobs() = 0;

        /**
         * @brief Method to process a job submission
         */
        virtual void processJobSubmission(std::shared_ptr<BatchJob> batch_job) = 0;

        /**
         * @brief Method to process a job failure
         */
        virtual void processJobFailure(std::shared_ptr<BatchJob> batch_job) = 0;

        /**
         * @brief Method to process a job completion
         */
        virtual void processJobCompletion(std::shared_ptr<BatchJob> batch_job) = 0;

        /**
         * @brief Method to process a job termination
         */
        virtual void processJobTermination(std::shared_ptr<BatchJob> batch_job) = 0;

        /**
         * @brief Method to get start time estimates
         */
        virtual std::map<std::string, double> getStartTimeEstimates(std::set<std::tuple<std::string, unsigned long, unsigned long, double>> set_of_jobs) = 0;



    protected:
        /**
         * @brief Compute service for which this scheduler is operating
         */
        BatchComputeService *cs;

    };

/***********************/
/** \endcond          **/
/***********************/
}


#endif //WRENCH_BATCHSCHEDULER_H
