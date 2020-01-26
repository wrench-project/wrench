/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_REDUCER_H
#define WRENCH_REDUCER_H

#include "wrench/services/compute/hadoop/MRJob.h"
#include "wrench/services/compute/ComputeService.h"

namespace wrench {

    class ReducerService : ComputeService {
    private:
        int main() override;

        bool processNextMessage();

    protected:
        MRJob &job;
    public:
        ReducerService(const std::string &hostname, MRJob &MRJob);

        ~ReducerService();

        MRJob &getJob() {
            return job;
        }

    };
}

#endif //WRENCH_REDUCER_H
