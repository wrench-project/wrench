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

#include <wrench/services/compute/hadoop/HadoopComputeService.h>
#include "wrench/services/compute/hadoop/MRJob.h"
#include "wrench/services/compute/hadoop/hadoop_subsystem/HdfsService.h"
#include "wrench/services/compute/ComputeService.h"

namespace wrench {
    class MapperService : public Service {
    public:
        MapperService(
                const std::string &hostname,
                MRJob *job,
                const std::set<std::string> compute_resources,
                std::map<std::string, std::string> property_list,
                std::map<std::string, double> messagepayload_list
        );

        void stop() override;
    private:
        MRJob *job;

        // TODO: Define these:
        std::map<std::string, std::string> default_property_values = {
                {HadoopComputeServiceProperty::MAP_SIDE_SPILL_PHASE, "0.0"},
                {HadoopComputeServiceProperty::MAP_SIDE_MERGE_PHASE, "0.0"}
        };

        // TODO: And define these:
        std::map<std::string, double> default_messagepayload_values = {
                {MRJobExecutorMessagePayload::MAP_SIDE_HDFS_DATA_DELIVERY_PAYLOAD, 1024},
                {MRJobExecutorMessagePayload::MAP_SIDE_HDFS_DATA_REQUEST_PAYLOAD,  1024},
                {MRJobExecutorMessagePayload::MAP_SIDE_SHUFFLE_REQUEST_PAYLOAD,    1024},
        };

        std::set<std::string> compute_resources;

        int main() override;

        bool processNextMessage();
    };
}
#endif //WRENCH_MAPPER_H
