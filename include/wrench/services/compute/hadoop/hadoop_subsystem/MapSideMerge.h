/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_MAPSIDEMERGE_H
#define WRENCH_MAPSIDEMERGE_H

#include "wrench/services/compute/hadoop/MRJob.h"

namespace wrench {

    class MapSideMerge {
    protected:
        MRJob &job;
    public:
        explicit MapSideMerge(MRJob &MRJob);

        ~MapSideMerge();

        double deterministicMapSideMergeIOCost();

        double deterministicMapSideMergCPUCost();

        void setJob(MRJob &job) {
            this->job = job;
        }

        MRJob &getJob() {
            return job;
        }

    };
}

#endif //WRENCH_MAPSIDEMERGE_H
