/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/hadoop/hadoop_subsystem/Shuffle.h"

namespace wrench {

    Shuffle::Shuffle(MRJob &MRJob) {
        this->setJobType(MRJob.getJobType());
    }

    double Shuffle::calculateShuffleCost() {
        if (this->getJobType() == std::string("deterministic")) {
            // TODO
        }
        return 0.0;
    }
}