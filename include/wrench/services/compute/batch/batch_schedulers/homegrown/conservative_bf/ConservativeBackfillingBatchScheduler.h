/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_CONSERVATIVEBACKFILLINGBATCHSCHEDULER_H
#define WRENCH_CONSERVATIVEBACKFILLINGBATCHSCHEDULER_H

#include "wrench/services/compute/batch/BatchComputeService.h"
#include "wrench/services/compute/batch/batch_schedulers/homegrown/HomegrownBatchScheduler.h"
#include <wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf/NodeAvailabilityTimeLine.h>

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief A class that defines a conservative backfilling batch scheduler
     */
    class ConservativeBackfillingBatchScheduler : public HomegrownBatchScheduler {

    public:
        explicit ConservativeBackfillingBatchScheduler(BatchComputeService *cs, unsigned long backfilling_depth);

        void processQueuedJobs() override;

        void processJobSubmission(std::shared_ptr<BatchJob> batch_job) override;
        void processJobFailure(std::shared_ptr<BatchJob> batch_job) override;
        void processJobCompletion(std::shared_ptr<BatchJob> batch_job) override;
        void processJobTermination(std::shared_ptr<BatchJob> batch_job) override;

        void compactSchedule();

        std::map<simgrid::s4u::Host *, std::tuple<unsigned long, sg_size_t>> scheduleOnHosts(unsigned long, unsigned long, sg_size_t) override;

        std::map<std::string, double>
        getStartTimeEstimates(std::set<std::tuple<std::string, unsigned long, unsigned long, sg_size_t>> set_of_jobs) override;

    private:
        unsigned long _backfilling_depth;
        std::unique_ptr<NodeAvailabilityTimeLine> schedule;
    };


    /***********************/
    /** \endcond           */
    /***********************/
}// namespace wrench


#endif//WRENCH_CONSERVATIVEBACKFILLINGBATCHSCHEDULER_H
