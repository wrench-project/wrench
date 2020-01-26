/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_DETERMINISTICMRJOB_H
#define WRENCH_DETERMINISTICMRJOB_H

#include "MRJob.h"

namespace wrench {

    class DeterministicMRJob : public MRJob {
    public:
        DeterministicMRJob(int num_mappers, int num_reducers, double data_size, bool use_combiner, int sort_factor,
                           double spill_percent, int key_value_width);

        DeterministicMRJob();

        ~DeterministicMRJob();

        std::pair<double, double> getMapCost(double map_function_cost) override;

        std::pair<double, double> getReduceCost(double reduce_function_cost) override;
    };
}

#endif //WRENCH_DETERMINISTICMRJOB_H
