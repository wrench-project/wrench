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
#include <utility>

namespace wrench {

    class DeterministicMRJob : public MRJob {
    private:
        std::vector<int> &files;
    public:

        DeterministicMRJob(
                int num_reducers, double data_size, int block_size,
                bool use_combiner, int sort_factor, double spill_percent,
                int mapper_key_width, int mapper_value_width, std::vector<int> &files,
                double mapper_flops, int reducer_key_width, int reducer_value_width,
                double reducer_flops
        );

        void setFiles(std::vector<int> files) {
            this->files = std::move(files);
        }

        std::vector<int> &getFiles() {
            return files;
        }

        int calculateNumMappers() override;

    };
}

#endif //WRENCH_DETERMINISTICMRJOB_H
