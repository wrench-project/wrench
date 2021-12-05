/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_CONSERVATIVEBFBATCHSCHEDULER_H
#define WRENCH_CONSERVATIVEBFBATCHSCHEDULER_H

#include "wrench/services/compute/batch/BatchComputeService.h"
#include "wrench/services/compute/batch/batch_schedulers/homegrown/HomegrownBatchScheduler.h"
#include <services/compute/batch/batch_schedulers/homegrown/conservative_bf/NodeAvailabilityTimeLine.h>

namespace wrench {

/***********************/
/** \cond INTERNAL     */
/***********************/

    /**
     * @brief A class that defines a conservative backfilling batch_standard_and_pilot_jobs scheduler
     */
    class CONSERVATIVEBFBatchScheduler : public HomegrownBatchScheduler {

    public:

        explicit CONSERVATIVEBFBatchScheduler(BatchComputeService *cs);

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

        std::unique_ptr<NodeAvailabilityTimeLine> schedule;
    };


/***********************/
/** \endcond           */
/***********************/
}



#endif //WRENCH_CONSERVATIVEBFBATCHSCHEDULER_H
