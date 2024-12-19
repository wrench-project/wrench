/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_CONSERVATIVEBACKFILLINGBATCHSCHEDULERCORELEVEL_H
#define WRENCH_CONSERVATIVEBACKFILLINGBATCHSCHEDULERCORELEVEL_H

#include "wrench/services/compute/batch/BatchComputeService.h"
#include "wrench/services/compute/batch/batch_schedulers/homegrown/HomegrownBatchScheduler.h"
#include "CoreAvailabilityTimeLine.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief A class that defines a conservative backfilling batch scheduler
     */
    class ConservativeBackfillingBatchSchedulerCoreLevel : public HomegrownBatchScheduler {

    public:
        explicit ConservativeBackfillingBatchSchedulerCoreLevel(BatchComputeService *cs, unsigned long backfilling_depth);

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
        std::unique_ptr<CoreAvailabilityTimeLine> schedule;
        unsigned long backfilling_depth;
    };


    /***********************/
    /** \endcond           */
    /***********************/
}// namespace wrench


#endif//WRENCH_CONSERVATIVEBACKFILLINGBATCHSCHEDULERCORELEVEL_H
