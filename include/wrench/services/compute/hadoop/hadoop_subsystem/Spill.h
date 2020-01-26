/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SPILL_H
#define WRENCH_SPILL_H

#include "wrench/services/compute/hadoop/MRJob.h"

namespace wrench {

    class Spill {
    protected:
        MRJob &job;
    public:

        explicit Spill(MRJob &MRJob);

        ~Spill();

        double deterministicSpillIOCost();

        double deterministicSpillCPUCost();

        void setJob(MRJob &job) {
            this->job = job;
        }

        MRJob &getJob() {
            return job;
        }

    };
}

#endif //WRENCH_SPILL_H
