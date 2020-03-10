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

    class BatschedBatchScheduler : public BatchScheduler {

    public:

        explicit BatschedBatchScheduler(BatchComputeService *cs) : BatchScheduler(cs) {}

        void init() override;
        void launch() override;
        void shutdown() override;

        void processQueuedJobs() override;

        void processJobFailure(BatchJob *batch_job, std::string job_id) override;
        void processJobCompletion(BatchJob *batch_job, std::string job_id) override;
        void processJobTermination(BatchJob *batch_job, std::string job_id) override;

        std::map<std::string, double> getStartTimeEstimates(std::set<std::tuple<std::string, unsigned int, unsigned int, double>> set_of_jobs) override;

    private:
#ifdef ENABLE_BATSCHED

        void notifyJobEventsToBatSched(std::string job_id, std::string status, std::string job_state,
                                       std::string kill_reason, std::string event_type);

        void appendJobInfoToCSVOutputFile(BatchJob *batch_job, std::string status);

        void startBatschedNetworkListener();

#endif

    };

}


#endif //WRENCH_BATSCHEDBATCHSCHEDULER_H
