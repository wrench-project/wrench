/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_EXAMPLE_TWO_TASKS_AT_A_TIME_H
#define WRENCH_EXAMPLE_TWO_TASKS_AT_A_TIME_H

#include <wrench-dev.h>


namespace wrench {

    class Simulation;

    /**
     *  @brief A Workflow Management System (WMS) implementation
     */
    class TwoTasksAtATimeWMS : public ExecutionController {

    public:
        // Constructor
        TwoTasksAtATimeWMS(
                std::shared_ptr<Workflow> workflow,
                std::shared_ptr<BareMetalComputeService> &bare_metal_compute_service,
                std::shared_ptr<SimpleStorageService> &storage_service,
                const std::string &hostname);

    protected:

        // Overridden method
        void processEventStandardJobCompletion(std::shared_ptr<StandardJobCompletedEvent>) override;
        void processEventStandardJobFailure(std::shared_ptr<StandardJobFailedEvent>) override;

    private:
        // main() method of the WMS
        int main() override;

        std::shared_ptr<Workflow> workflow;
        std::shared_ptr<BareMetalComputeService> bare_metal_compute_service;
        std::shared_ptr<SimpleStorageService> storage_service;


    };
}
#endif //WRENCH_EXAMPLE_TWO_TASKS_AT_A_TIME_H
