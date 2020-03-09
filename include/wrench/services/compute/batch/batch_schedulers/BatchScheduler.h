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

        BatchScheduler(BatchComputeService *cs) : cs(cs) {};

        virtual std::map<std::string, double> getStartTimeEstimates(std::set<std::tuple<std::string, unsigned int, unsigned int, double>> set_of_jobs) = 0;

        virtual BatchJob *pickNextJobToSchedule() = 0;

        virtual std::map<std::string, std::tuple<unsigned long, double>> scheduleOnHosts(unsigned long, unsigned long, double) = 0;


    protected:
        // Batch Compute service this scheduler is for
        BatchComputeService *cs;

    };
}

/***********************/
/** \endcond          **/
/***********************/


#endif //WRENCH_BATCHSCHEDULER_H
