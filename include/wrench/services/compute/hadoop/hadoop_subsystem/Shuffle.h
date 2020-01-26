/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SHUFFLE_H
#define WRENCH_SHUFFLE_H

#include "wrench/services/compute/hadoop/MRJob.h"

namespace wrench {

    class Shuffle {
    private:
        std::string job_type;
    public:
        explicit Shuffle(MRJob &MRJob);

        double calculateShuffleCost();

        void setJobType(std::string &job_type) {
            this->job_type = job_type;
        }

        std::string &getJobType() {
            return job_type;
        }
    };
}

#endif //WRENCH_SHUFFLE_H
