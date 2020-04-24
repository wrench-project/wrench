/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/hadoop/DeterministicMRJob.h"
#include "wrench/services/compute/hadoop/hadoop_subsystem/MapperService.h"
#include "wrench/logging/TerminalOutput.h"

WRENCH_LOG_NEW_DEFAULT_CATEGORY(hadoop_compute_servivce, "Log category for Deterministic MR Job");

namespace wrench {

    DeterministicMRJob::DeterministicMRJob(
            int num_mappers, int num_reducers, double data_size,
            bool use_combiner, int sort_factor, double spill_percent,
            int mapper_key_width, int mapper_value_width,
            double mapper_flops, int reducer_key_width, int reducer_value_width,
            double reducer_flops
    ) {
        this->setNumMappers(num_mappers);
        this->setNumReducers(num_reducers);
        this->setUseCombiner(use_combiner);
        this->setDataSize(data_size);
        this->setSortFactor(sort_factor);
        this->setSpillPercent(spill_percent);
        this->setMapperKeyWidth(mapper_key_width);
        this->setMapperValueWidth(mapper_value_width);
        this->setMapperFlops(mapper_flops);
        this->setReducerKeyWidth(reducer_key_width);
        this->setReducerValueWidth(reducer_value_width);
        this->setReducerFlops(reducer_flops);
        this->setJobType(std::string("deterministic"));
    }

    // Typical set of parameters
    DeterministicMRJob::DeterministicMRJob() {
        this->setNumMappers(10);
        this->setNumReducers(1);
        this->setUseCombiner(false);
        this->setDataSize(1048576);  // 2^20 bytes
        this->setSortFactor(10);
        this->setSpillPercent(0.8);
        this->setMapperKeyWidth(32);
        this->setJobType(std::string("deterministic"));
    };

    DeterministicMRJob::~DeterministicMRJob() = default;

}
