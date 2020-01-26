/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_HDFS_H
#define WRENCH_HDFS_H

#include "wrench/services/compute/hadoop/MRJob.h"
#include "wrench/services/compute/ComputeService.h"

namespace wrench {

    class HdfsService : ComputeService {
    private:
        bool processNextMessage();
        int main();
    protected:
        MRJob &job;
        double read_cost;
        double write_cost;
    public:
        HdfsService(const std::string &hostname, MRJob &MRJob,
                double read_cost, double write_cost);

        ~HdfsService();

        double deterministicReadIOCost();

        double deterministicReadCPUCost();

        double deterministicWriteIOCost();

        double deterministicWriteCPUCost();

        void setJob(MRJob &job) {
            this->job = job;
        }

        void setReadCost(double cost) {
            this->read_cost = cost;
        }

        void setWriteCost(double cost) {
            this->write_cost = cost;
        }

        double getReadCost() {
            return read_cost;
        }

        double getWriteCost() {
            return write_cost;
        }

        MRJob &getJob() {
            return job;
        }
    };
}

#endif //WRENCH_HDFS_H
