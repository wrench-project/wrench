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

#include <wrench/services/compute/hadoop/HadoopComputeServiceProperty.h>
#include "wrench/services/compute/hadoop/MRJob.h"
#include "wrench/services/compute/ComputeService.h"

namespace wrench {
    class HdfsService : public Service {
    public:
        HdfsService(
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
                {HadoopComputeServiceProperty::HDFS_READ, "0.0"},
                {HadoopComputeServiceProperty::HDFS_WRITE, "0.0"}
        };

        // TODO: And define these:
        std::map<std::string, double> default_messagepayload_values = {};

        std::set<std::string> compute_resources;

        int main() override;

        bool processNextMessage();
    };
}
#endif //WRENCH_HDFS_H
