/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_HOMEGROWNBATCHSCHEDULER_H
#define WRENCH_HOMEGROWNBATCHSCHEDULER_H

#include "wrench/services/compute/batch/batch_schedulers/BatchScheduler.h"

namespace wrench {

    class HomegrownBatchScheduler : public BatchScheduler {

    public:

        explicit HomegrownBatchScheduler(BatchComputeService *cs) : BatchScheduler(cs) {}

        void init() override {};

        void launch() override {};

        void shutdown() override {};

        virtual std::map<std::string, std::tuple<unsigned long, double>> scheduleOnHosts(unsigned long, unsigned long, double) = 0;

    };

}

#endif //WRENCH_HOMEGROWNBATCHSCHEDULER_H
