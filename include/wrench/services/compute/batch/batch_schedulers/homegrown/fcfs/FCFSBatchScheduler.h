/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FCFSBATCHSCHEDULER_H
#define WRENCH_FCFSBATCHSCHEDULER_H

/***********************/
/** \cond INTERNAL     */
/***********************/

#include "wrench/services/compute/batch/batch_schedulers/homegrown/HomegrownBatchScheduler.h"
#include "wrench/services/compute/batch/BatchComputeService.h"

namespace wrench {

    /**
     * @brief A class that implements a FCFS batch scheduler
     */
    class FCFSBatchScheduler : public HomegrownBatchScheduler {

    public:
        /**
         * @brief Constructor
         *
         * @param cs: the batch compute service for which this scheduler is operating
         */
        explicit FCFSBatchScheduler(BatchComputeService *cs) : HomegrownBatchScheduler(cs) {}

        void processQueuedJobs() override;

        void processJobSubmission(std::shared_ptr<BatchJob> batch_job) override;
        void processJobFailure(std::shared_ptr<BatchJob> batch_job) override;
        void processJobCompletion(std::shared_ptr<BatchJob> batch_job) override;
        void processJobTermination(std::shared_ptr<BatchJob> batch_job) override;

        std::shared_ptr<BatchJob> pickNextJobToSchedule() const;

        std::map<simgrid::s4u::Host *, std::tuple<unsigned long, sg_size_t>> scheduleOnHosts(unsigned long, unsigned long, sg_size_t) override;

        std::map<std::string, double> getStartTimeEstimates(std::set<std::tuple<std::string, unsigned long, unsigned long, sg_size_t>> set_of_jobs) override;
    };

}// namespace wrench

/***********************/
/** \endcond           */
/***********************/


#endif//WRENCH_FCFSBATCHSCHEDULER_H
