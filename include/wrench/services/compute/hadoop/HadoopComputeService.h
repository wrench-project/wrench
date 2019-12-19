/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_HADOOPCOMPUTESERVICE_H
#define WRENCH_HADOOPCOMPUTESERVICE_H

#include  <string>
#include  <set>
#include  <map>

#include "wrench/services/compute/ComputeService.h"
#include "HadoopComputeServiceProperty.h"
#include "HadoopComputeServiceMessagePayload.h"

namespace wrench {


    class HadoopComputeService : public ComputeService {

    public:

        HadoopComputeService(
                const std::string &hostname,
                const std::set<std::string> compute_resources,
                std::map<std::string, std::string> property_list,
                std::map<std::string, double> messagepayload_list
        );

        void stop();

        void runMRJob();

    private:

        void
        submitStandardJob(StandardJob *job, std::map<std::string, std::string> &service_specific_arguments)  {
            throw std::runtime_error("HadoopComputeService::submitStandardJob(): not implemented");
        };


        void submitPilotJob(PilotJob *job, std::map<std::string, std::string> &service_specific_arguments) {
            throw std::runtime_error("HadoopComputeService::submitPilotJob(): not implemented");
        };

        void terminateStandardJob(StandardJob *job) {
            throw std::runtime_error("HadoopComputeService::terminateStandardJob(): not implemented");
        };

        void terminatePilotJob(PilotJob *job) {
            throw std::runtime_error("HadoopComputeService::terminatePilotJob(): not implemented");
        };


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
