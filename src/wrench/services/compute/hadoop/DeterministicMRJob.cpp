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

    DeterministicMRJob::DeterministicMRJob(int num_mappers, int num_reducers, double data_size,
                                           bool use_combiner, int sort_factor, double spill_percent,
                                           int key_value_width) : MRJob() {
        this->setNumMappers(num_mappers);
        this->setNumReducers(num_reducers);
        this->setUseCombiner(use_combiner);
        this->setDataSize(data_size);
        this->setSortFactor(sort_factor);
        this->setSpillPercent(spill_percent);
        this->setKeyValueWidth(key_value_width);
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
        this->setKeyValueWidth(32);
        this->setJobType(std::string("deterministic"));
    };

    DeterministicMRJob::~DeterministicMRJob() = default;

    std::pair<double, double> DeterministicMRJob::getMapCost(double map_function_cost) {
        /** TODO:
         * Using the deterministic model, the map cost is as simple
         * as:
         *    num_mappers * total_map_cost
         * A Mapper object will have the following
         * sub-costs:
         *  - Read: Via an `HDFS` object
         *  - User map function cost: Unsure
         *  - Collection and spill: Via a `Spill` object
         *  - Merge: Via a `MapSideMerge` object
         */

        auto mapper = new MapperService(*this, map_function_cost);
        // returns <map_cpu_cost, map_io_cost>
        return mapper->calculateMapperCost();
    }

    std::pair<double, double> DeterministicMRJob::getReduceCost(double reduce_function_cost) {
        /** TODO:
         * Similary, the same idea can be used on the reduce side:
         *  - Shuffle: Via a `Shuffle` object
         *  - Merge: Via a `ReduceSideMerge` object
         *  - Reduce: Unsure
         *  - Write: Via an `HDFS` object
         */
        return std::make_pair(0.0, 0.0);
    }
}
