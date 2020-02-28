/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_MRJOBNEXECUTOR_H
#define WRENCH_MRJOBNEXECUTOR_H

#include  <string>
#include  <set>
#include  <map>

#include "wrench/services/Service.h"
#include "wrench/services/compute/hadoop/hadoop_subsystem/MRJobExecutor"
#include "HadoopComputeServiceProperty.h"
#include "HadoopComputeServiceMessagePayload.h"
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

        void stop();

    private:

        MRJob *job;
        std::string notify_mailbox;

        std::map<std::string, std::string> default_property_values = {
                {HadoopComputeServiceProperty::MAP_STARTUP_OVERHEAD,                         "0.0"},
        };

        std::map<std::string, double> default_messagepayload_values = {
                {HadoopComputeServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 1024},
                {HadoopComputeServiceMessagePayload::RUN_MR_JOB_REQUEST_MESSAGE_PAYLOAD, 1024},
                {HadoopComputeServiceMessagePayload::RUN_MR_JOB_ANSWER_MESSAGE_PAYLOAD, 1024},
        };

        std::set<std::string> compute_resources;

        int main();

        bool processNextMessage();


    };
}


#endif //WRENCH_HADOOPCOMPUTESERVICE_H
