/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_MRJOB_H
#define WRENCH_MRJOB_H

#include <string>
#include <vector>

namespace wrench {

    class MRJob {
    private:

        int num_mappers;
        int num_reducers;
        int sort_factor;
        int mapper_key_width;
        int mapper_value_width;
        double mapper_flops;
        int reducer_key_width;
        int reducer_value_width;
        double reducer_flops;
        double spill_percent;
        bool use_combiner;
        double data_size;
        int block_size;
        std::string job_type;
        std::string hdfs_mailbox_name;

    public:

        virtual ~MRJob() = default;

        virtual int calculateNumMappers() = 0;

        // Mutators
        void setBlockSize(int block_size) {
            this->block_size = block_size;
        }

        void setNumMappers(int num_mappers) {
            this->num_mappers = num_mappers;
        }
        void setNumReducers(int num_reducers) {
            this->num_reducers = num_reducers;
        }

        void setSpillPercent(double spill_percent) {
            this->spill_percent = spill_percent;
        }

        void setSortFactor(int factor) {
            this->sort_factor = factor;
        }

        void setUseCombiner(bool use_combiner) {
            this->use_combiner = use_combiner;
        }

        void setDataSize(double data_size) {
            this->data_size = data_size;
        }

        void setMapperFlops(double flops) {
            this->mapper_flops = flops;
        }

        void setMapperKeyWidth(int width) {
            this->mapper_key_width = width;
        }

        void setMapperValueWidth(int width) {
            this->mapper_value_width = width;
        }

        void setReducerFlops(double flops) {
            this->reducer_flops = flops;
        }

        void setReducerKeyWidth(int width) {
            this->reducer_key_width = width;
        }

        void setReducerValueWidth(int width) {
            this->reducer_value_width = width;
        }

        void setJobType(std::string job_type) {
            this->job_type = job_type;
        }

        void setHdfsMailboxName(std::string mailbox_name) {
            this->hdfs_mailbox_name = mailbox_name;
        }

        // Accessors

        int getBlockSize() {
            return block_size;
        }

        int getNumMappers() {
            return num_mappers;
        }

        int getNumReducers() {
            return num_reducers;
        }

        double getSpillPercent() {
            return spill_percent;
        }

        int getSortFactor() {
            return sort_factor;
        }

        bool getUseCombiner() {
            return use_combiner;
        }

        double getDataSize() {
            return data_size;
        }

        std::string &getJobType() {
            return job_type;
        }

        double getMapperFlops() {
            return mapper_flops;
        }

        int getMapperKeyWidth() {
            return mapper_key_width;
        }

        int getMapperValueWidth() {
            return mapper_value_width;
        }

        double getReducerFlops() {
            return reducer_flops;
        }

        int getReducerKeyWidth() {
            return reducer_key_width;
        }

        int getReducerValueWidth() {
            return reducer_value_width;
        }

        std::string getHdfsMailboxName() {
            return hdfs_mailbox_name;
        }
    };
}

#endif //WRENCH_MRJOB_H
