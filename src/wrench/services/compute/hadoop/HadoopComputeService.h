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

    private:

        std::map<std::string, std::string> default_property_values = {
                {HadoopComputeServiceProperty::MAP_STARTUP_OVERHEAD,                         "0.0"},
        };

        std::map<std::string, double> default_messagepayload_values = {
                {HadoopComputeServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 1024},
        };

        std::set<std::string> compute_resources;

        int main();

        bool processNextMessage();


    };
}


#endif //WRENCH_HADOOPCOMPUTESERVICE_H
