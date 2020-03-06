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

namespace wrench {

    class MRJob {
    private:

        int num_mappers;
        int num_reducers;
        int sort_factor;
        int key_value_width;
        double spill_percent;
        bool use_combiner;
        double data_size;
        std::string job_type;

    public:

        // Mutators
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

        void setKeyValueWidth(int width) {
            this->key_value_width = width;
        }

        void setJobType(std::string job_type) {
            this->job_type = job_type;
        }

        // Accessors
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

        int getKeyValueWidth() {
            return key_value_width;
        }
    };
}

#endif //WRENCH_MRJOB_H
