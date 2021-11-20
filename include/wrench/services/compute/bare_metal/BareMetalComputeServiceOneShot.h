/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_BAREMETALCOMPUTESERVICEONESHOT_H
#define WRENCH_BAREMETALCOMPUTESERVICEONESHOT_H


#include <queue>

#include "wrench/services/compute/bare_metal/BareMetalComputeService.h"
#include "BareMetalComputeServiceProperty.h"
#include "BareMetalComputeServiceMessagePayload.h"



namespace wrench {

    class Simulation;
    class StorageService;
    class FailureCause;
    class Alarm;
    class Action;
    class ActionExecutionService;


    /**
     * @brief A bare-metal compute service that only runs one job, provided to its constructor
     */
    class BareMetalComputeServiceOneShot : public BareMetalComputeService {

        friend class BatchComputeService;

    public:

    protected:

    private:

        friend class Simulation;
        friend class BatchComputeService;

        BareMetalComputeServiceOneShot(std::shared_ptr<CompoundJob> job,
                                       const std::string &hostname,
                                       std::map<std::string, std::tuple<unsigned long, double>> compute_resources,
                                       std::map<std::string, std::string> property_list,
                                       std::map<std::string, double> messagepayload_list,
                                       double ttl,
                                       std::shared_ptr<PilotJob> pj,
                                       std::string suffix,
                                       std::shared_ptr<StorageService> scratch_space); // reference to upper level scratch space

        int main() override;

        std::shared_ptr<CompoundJob> job;

    };
};


#endif //WRENCH_BAREMETALCOMPUTESERVICEONESHOT_H
