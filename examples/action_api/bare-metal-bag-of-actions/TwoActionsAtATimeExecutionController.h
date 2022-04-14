/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_EXAMPLE_TWO_ACTIONS_AT_A_TIME_H
#define WRENCH_EXAMPLE_TWO_ACTIONS_AT_A_TIME_H

#include <wrench-dev.h>


namespace wrench {

    class Simulation;

    /**
     *  @brief An execution controller implementation
     */
    class TwoActionsAtATimeExecutionController : public ExecutionController {

    public:
        // Constructor
        TwoActionsAtATimeExecutionController(
                int num_actions,
                std::shared_ptr<BareMetalComputeService>  compute_service,
                std::shared_ptr<SimpleStorageService>  storage_service,
                const std::string &hostname);

    protected:
        // Overridden method
        void processEventCompoundJobCompletion(std::shared_ptr<CompoundJobCompletedEvent>) override;
        void processEventCompoundJobFailure(std::shared_ptr<CompoundJobFailedEvent>) override;

    private:
        // main() method of the WMS
        int main() override;

        const std::shared_ptr<BareMetalComputeService> compute_service;
        const std::shared_ptr<SimpleStorageService> storage_service;

        int num_actions;
    };
}// namespace wrench
#endif//WRENCH_EXAMPLE_TWO_ACTIONS_AT_A_TIME_H
