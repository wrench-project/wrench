/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef STRESS_TEST_ACTION_API_CONTROLLER_H
#define STRESS_TEST_ACTION_API_CONTROLLER_H

#include <wrench-dev.h>

#include <utility>

namespace wrench {

    class StressTestActionAPIController : public ExecutionController {

    public:
        //        ~StressTestActionAPIController() {
        //            this->compute_services.clear();
        //            this->storage_services.clear();
        //        }

        StressTestActionAPIController(std::set<std::shared_ptr<ComputeService>> compute_services,
                                      std::set<std::shared_ptr<StorageService>> storage_services,
                                      std::set<std::shared_ptr<NetworkProximityService>> network_proximity_services,
                                      int num_jobs,
                                      const std::string &hostname) : ExecutionController(hostname, "stresstestwms"),
                                                                     compute_services(std::move(compute_services)),
                                                                     storage_services(std::move(storage_services)),
                                                                     network_proximity_services(std::move(network_proximity_services)),
                                                                     num_jobs(num_jobs) {}

        int main() override;

    private:
        std::set<std::shared_ptr<ComputeService>> compute_services;
        std::set<std::shared_ptr<StorageService>> storage_services;
        std::set<std::shared_ptr<NetworkProximityService>> network_proximity_services;
        unsigned long num_jobs;
    };

};// namespace wrench


#endif//STRESS_TEST_ACTION_API_CONTROLLER_H
