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
            int num_reducers, double data_size, int block_size,
            bool use_combiner, int sort_factor, double spill_percent,
            int mapper_key_width, int mapper_value_width, std::vector<int> &files,
            double mapper_flops, int reducer_key_width, int reducer_value_width,
            double reducer_flops
    ) : files(files) {
        this->setBlockSize(block_size);
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

    DeterministicMRJob::~DeterministicMRJob() = default;

    int DeterministicMRJob::calculateNumMappers() {
        /*
         * Finding the total number of mappers for a given job.
         *
         * number_of_map_tasks = sum(ceil(s_i / b))
         * Where N is the number of files, s_i is the size of the ith file, and b is the block size.
         */
        int total_mappers = 0;
        for (auto file: this->getFiles()) {
            if (file < this->getBlockSize()) {
                total_mappers += 1;
            } else {
                int file_copy = file;
                while (file > this->getBlockSize()) {
                    file_copy -= this->getBlockSize();
                    total_mappers += 1;
                }
                total_mappers += 1;
            }
        }
        this->setNumMappers(total_mappers);
        return this->getNumMappers();
    }

}
