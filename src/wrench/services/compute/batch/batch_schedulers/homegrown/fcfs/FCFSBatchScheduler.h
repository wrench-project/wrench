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

#include <wrench/services/compute/batch/batch_schedulers/homegrown/HomegrownBatchScheduler.h>
#include <wrench/services/compute/batch/BatchComputeService.h>

namespace wrench {

    class FCFSBatchScheduler : public HomegrownBatchScheduler {

    public:

        explicit FCFSBatchScheduler(BatchComputeService *cs) : HomegrownBatchScheduler(cs) {}

        void processQueuedJobs() override;

        // TODO: is't the second arg just inside the first->id field?
        void processJobSubmission(BatchJob *batch_job) override;
        void processJobFailure(BatchJob *batch_job) override;
        void processJobCompletion(BatchJob *batch_job) override;
        void processJobTermination(BatchJob *batch_job) override;

        BatchJob *pickNextJobToSchedule() override;

        std::map <std::string, std::tuple<unsigned long, double>> scheduleOnHosts(unsigned long, unsigned long, double) override;

        std::map<std::string, double> getStartTimeEstimates(std::set <std::tuple<std::string, unsigned int, unsigned int, double>> set_of_jobs) override;

    };

}

/***********************/
/** \endcond           */
/***********************/



#endif //WRENCH_FCFSBATCHSCHEDULER_H
