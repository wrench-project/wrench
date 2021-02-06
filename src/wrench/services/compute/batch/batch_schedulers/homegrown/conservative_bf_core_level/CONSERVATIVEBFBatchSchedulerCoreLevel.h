/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_CONSERVATIVEBFBATCHSCHEDULERCORELEVEL_H
#define WRENCH_CONSERVATIVEBFBATCHSCHEDULERCORELEVEL_H

#include <wrench/services/compute/batch/BatchComputeService.h>
#include <wrench/services/compute/batch/batch_schedulers/homegrown/HomegrownBatchScheduler.h>
#include "CoreAvailabilityTimeLine.h"

namespace wrench {

/***********************/
/** \cond INTERNAL     */
/***********************/

    /**
     * @brief A class that defines a conservative backfilling batch scheduler
     */
    class CONSERVATIVEBFBatchSchedulerCoreLevel : public HomegrownBatchScheduler {

    public:

        explicit CONSERVATIVEBFBatchSchedulerCoreLevel(BatchComputeService *cs);

        // TODO: IMPLEMENT EVERYTHING
        void processQueuedJobs() override;

        void processJobSubmission(std::shared_ptr<BatchJob> batch_job) override;
        void processJobFailure(std::shared_ptr<BatchJob> batch_job) override;
        void processJobCompletion(std::shared_ptr<BatchJob> batch_job) override;
        void processJobTermination(std::shared_ptr<BatchJob> batch_job) override;

        void compactSchedule();

        std::map <std::string, std::tuple<unsigned long, double>> scheduleOnHosts(unsigned long, unsigned long, double) override;

        std::map<std::string, double>
        getStartTimeEstimates(std::set <std::tuple<std::string, unsigned long, unsigned long, double>> set_of_jobs) override;

    private:

        std::unique_ptr<CoreAvailabilityTimeLine> schedule;
    };


/***********************/
/** \endcond           */
/***********************/
}



#endif //WRENCH_CONSERVATIVEBFBATCHSCHEDULERCORELEVEL_H
