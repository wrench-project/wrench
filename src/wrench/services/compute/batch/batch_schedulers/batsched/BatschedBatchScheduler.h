/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_BATSCHEDBATCHSCHEDULER_H
#define WRENCH_BATSCHEDBATCHSCHEDULER_H

#include "wrench/services/compute/batch/batch_schedulers/BatchScheduler.h"

namespace wrench {

/***********************/
/** \cond INTERNAL     */
/***********************/

    /**
     * @brief A class that defines a batsched batch scheduler
     */
    class BatschedBatchScheduler : public BatchScheduler {

    public:

        /**
         * @brief Constructor
         * @param cs: The computer service for which this scheduler is operating
         */
        explicit BatschedBatchScheduler(BatchComputeService *cs) : BatchScheduler(cs) {}

        /**
         * @brief Initialization method
         */
        void init() override;

        /**
         * @brief Launch method
         */
        void launch() override;

        /**
         * @brief Shutdown method
         */
        void shutdown() override;

        /**
         * @brief Method to process queued jobs
         */
        void processQueuedJobs() override;

        /**
         * @brief Method to process a job submission
         */
        void processJobSubmission(std::shared_ptr<BatchJob> batch_job) override;

        /**
         * @brief Method to process a job failure
         */
        void processJobFailure(std::shared_ptr<BatchJob> batch_job) override;

        /**
         * @brief Method to process a job completion
         */
        void processJobCompletion(std::shared_ptr<BatchJob> batch_job) override;

        /**
         * @brief Method to process a job termination
         */
        void processJobTermination(std::shared_ptr<BatchJob> batch_job) override;

        /**
         * @brief Method to get start time estimates
         */
        std::map<std::string, double> getStartTimeEstimates(std::set<std::tuple<std::string, unsigned long, unsigned long, double>> set_of_jobs) override;

    private:
#ifdef ENABLE_BATSCHED

        void notifyJobEventsToBatSched(std::string job_id, std::string status, std::string job_state,
                                       std::string kill_reason, std::string event_type);

        void appendJobInfoToCSVOutputFile(BatchJob *batch_job, std::string status);

        void startBatschedNetworkListener();


        pid_t pid;

        unsigned long batsched_port;


#endif

    };

/***********************/
/** \endcond           */
/***********************/

}


#endif //WRENCH_BATSCHEDBATCHSCHEDULER_H
