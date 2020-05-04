/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_MRJOBEXECUTOR_H
#define WRENCH_MRJOBEXECUTOR_H

#include  <string>
#include  <set>
#include  <map>

#include "wrench/services/Service.h"
#include "wrench/services/compute/hadoop/HadoopComputeServiceProperty.h"
#include "wrench/services/compute/hadoop/hadoop_subsystem/MRJobExecutorMessagePayload.h"
#include "wrench/services/compute/hadoop/MRJob.h"

namespace wrench {
    class MRJobExecutor : public Service {
    public:
        MRJobExecutor(
                const std::string &hostname,
                MRJob *job,
                const std::set<std::string> compute_resources,
                std::string notify_mailbox,
                std::map<std::string, std::string> property_list,
                std::map<std::string, double> messagepayload_list
        );

        void stop() override;

    private:
        MRJob *job;
        std::string notify_mailbox;
        bool success;
        std::vector<std::shared_ptr<Service>> mappers;
        std::vector<std::shared_ptr<Service>> reducers;
        std::shared_ptr<Service> hdfs;
        std::shared_ptr<Service> shuffle;

        std::map<std::string, std::string> default_property_values = {
                {HadoopComputeServiceProperty::MAP_STARTUP_OVERHEAD,     "0.0"},
                {HadoopComputeServiceProperty::REDUCER_STARTUP_OVERHEAD, "0.0"},
        };

        std::map<std::string, double> default_messagepayload_values = {
                {MRJobExecutorMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD,            1024},
                {MRJobExecutorMessagePayload::NOTIFY_EXECUTOR_STATUS_MESSAGE_PAYLOAD, 1024},
                {MRJobExecutorMessagePayload::MAP_SIDE_SHUFFLE_REQUEST_PAYLOAD,       1024},
                {MRJobExecutorMessagePayload::MAP_SIDE_HDFS_DATA_DELIVERY_PAYLOAD,    1024},
                {MRJobExecutorMessagePayload::MAP_SIDE_HDFS_DATA_REQUEST_PAYLOAD,     1024},
        };

        std::set<std::string> compute_resources;

        int main() override;

        bool processNextMessage();

        void setup_workers(std::vector<std::shared_ptr<Service>> &mappers,
                           std::vector<std::shared_ptr<Service>> &reducers,
                           std::shared_ptr<Service> &hdfs,
                           std::shared_ptr<Service> &shuffle);
    };
}

#endif //WRENCH_HADOOPCOMPUTESERVICE_H
