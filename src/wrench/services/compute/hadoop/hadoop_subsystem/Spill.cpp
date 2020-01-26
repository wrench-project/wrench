/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/hadoop/hadoop_subsystem/Spill.h"

namespace wrench {

    Spill::Spill(MRJob &MRJob) : job(MRJob) {

    }

    Spill::~Spill() = default;

    double Spill::deterministicSpillIOCost() {
        // TODO
        return 0.0;
    }

    double Spill::deterministicSpillCPUCost() {
        // TODO
        return 0.0;
    }
}