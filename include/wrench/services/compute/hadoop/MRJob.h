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
        int reducer_key_width;
        int reducer_value_width;
        int block_size;
        double mapper_flops;
        double reducer_flops;
        double spill_percent;
        double data_size;
        bool use_combiner;
        std::string job_type;
        std::string hdfs_mailbox_name;
        std::string executor_mailbox;
        std::string shuffle_mailbox;
        std::vector<std::string> reducer_mailboxes;

    public:
        virtual ~MRJob() = default;

        virtual int calculateNumMappers() = 0;

        // Mutators

        void appendReducerMailbox(std::string reducer_mailbox) {
            this->reducer_mailboxes.push_back(reducer_mailbox);
        }

        void setShuffleMailbox(std::string shuffle_mailbox) {
            this->shuffle_mailbox = shuffle_mailbox;
        }

        void setExecutorMailbox(std::string executor_mailbox) {
            this->executor_mailbox = executor_mailbox;
        }

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
            this->job_type = std::move(job_type);
        }

        void setHdfsMailboxName(std::string mailbox_name) {
            this->hdfs_mailbox_name = std::move(mailbox_name);
        }

        // Accessors

        std::vector<std::string> getReducerMailboxes() {
            return reducer_mailboxes;
        }

        std::string getShuffleMailbox() {
            return shuffle_mailbox;
        }

        std::string getExecutorMailbox() {
            return executor_mailbox;
        }

        int getBlockSize() const {
            return block_size;
        }

        int getNumMappers() const {
            return num_mappers;
        }

        int getNumReducers() const {
            return num_reducers;
        }

        double getSpillPercent() const {
            return spill_percent;
        }

        int getSortFactor() const {
            return sort_factor;
        }

        bool getUseCombiner() const {
            return use_combiner;
        }

        double getDataSize() const {
            return data_size;
        }

        std::string &getJobType() {
            return job_type;
        }

        double getMapperFlops() const {
            return mapper_flops;
        }

        int getMapperKeyWidth() const {
            return mapper_key_width;
        }

        int getMapperValueWidth() const {
            return mapper_value_width;
        }

        double getReducerFlops() const {
            return reducer_flops;
        }

        int getReducerKeyWidth() const {
            return reducer_key_width;
        }

        int getReducerValueWidth() const {
            return reducer_value_width;
        }

        std::string getHdfsMailboxName() {
            return hdfs_mailbox_name;
        }
    };
}

#endif //WRENCH_MRJOB_H
