/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "CONSERVATIVE_BFBatchScheduler.h"

namespace wrench {

    CONSERVATIVE_BFBatchScheduler::CONSERVATIVE_BFBatchScheduler(BatchComputeService *cs) : HomegrownBatchScheduler(cs) {
        this->schedule = std::unique_ptr<NodeAvailabilityTimeLine>(new NodeAvailabilityTimeLine(cs->total_num_of_nodes));
    }


}
