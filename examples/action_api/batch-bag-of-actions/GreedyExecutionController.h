/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_EXAMPLE_GREEDY_EXECUTION_CONTROLLER_H
#define WRENCH_EXAMPLE_GREEDY_EXECUTION_CONTROLLER_H

#include <wrench-dev.h>


namespace wrench {

    class Simulation;

    /**
     *  @brief An execution controller implementation
     */
    class GreedyExecutionController : public ExecutionController {

    public:
        // Constructor
        GreedyExecutionController(
                int num_actions,
                std::shared_ptr<BatchComputeService> compute_service,
                std::shared_ptr<SimpleStorageService> storage_service,
                const std::string &hostname);

    private:
        // main() method of the WMS
        int main() override;

        const std::shared_ptr<BatchComputeService> compute_service;
        const std::shared_ptr<SimpleStorageService> storage_service;

        std::shared_ptr<PilotJob> pilot_job;

        int num_actions;
    };
}// namespace wrench
#endif//WRENCH_EXAMPLE_GREEDY_EXECUTION_CONTROLLER_H
