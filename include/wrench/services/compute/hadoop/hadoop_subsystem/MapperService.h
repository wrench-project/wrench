/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_MAPPER_H
#define WRENCH_MAPPER_H

#include "wrench/services/compute/hadoop/MRJob.h"
#include "wrench/services/compute/hadoop/hadoop_subsystem/HdfsService.h"
#include "wrench/services/compute/hadoop/hadoop_subsystem/Spill.h"
#include "wrench/services/compute/hadoop/hadoop_subsystem/MapSideMerge.h"
#include "wrench/services/compute/ComputeService.h"

namespace wrench {

    class MapperService : ComputeService {
    private:
        int main();

        bool processNextMessage();

    protected:
        MRJob &job;
        double map_function_cost;
    public:
        MapperService(const std::string &hostname, MRJob &MRJob, double map_function_cost);

        ~MapperService();

        std::pair<double, double> calculateMapperCost();

        void setJob(MRJob &job) {
            this->job = job;
        }

        void setMapFunctionCost(double map_function_cost) {
            this->map_function_cost = map_function_cost;
        }

        MRJob &getJob() {
            return job;
        }

        double getMapFunctionCost() {
            return map_function_cost;
        }
    };
}

#endif //WRENCH_MAPPER_H
