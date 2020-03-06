/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SHUFFLE_H
#define WRENCH_SHUFFLE_H

#include "wrench/services/compute/hadoop/MRJob.h"
#include "wrench/services/compute/hadoop/HadoopComputeService.h"

namespace wrench {

    class ShuffleService : public Service {
    public:

        ShuffleService(
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
        std::map<std::string, std::string> default_property_values = {};

        // TODO: And define these:
        std::map<std::string, double> default_messagepayload_values = {};

        std::set<std::string> compute_resources;

        int main() override;

        bool processNextMessage();

    };
}

#endif //WRENCH_SHUFFLE_H
