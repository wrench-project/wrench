/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/hadoop/hadoop_subsystem/MapSideMerge.h"

namespace wrench {

    MapSideMerge::MapSideMerge(MRJob &MRJob) : job(MRJob) {

    }

    MapSideMerge::~MapSideMerge() = default;

    double MapSideMerge::deterministicMapSideMergeIOCost() {
        // TODO
        return 0.0;
    }

    double MapSideMerge::deterministicMapSideMergCPUCost() {
        // TODO
        return 0.0;
    }
}