/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_EXAMPLE_TWO_TASKS_AT_A_TIME_BATCH_H
#define WRENCH_EXAMPLE_TWO_TASKS_AT_A_TIME_BATCH_H

#include <wrench-dev.h>


namespace wrench {

    class Simulation;

    /**
     *  @brief A Workflow Management System (WMS) implementation
     */
    class TwoTasksAtATimeBatchWMS : public ExecutionController {

    public:
        // Constructor
        TwoTasksAtATimeBatchWMS(
                std::shared_ptr<Workflow> workflow,
                const std::shared_ptr<BatchComputeService> &batch_compute_service,
                const std::shared_ptr<StorageService> &storage_service,
                const std::string &hostname);

    private:
        // main() method of the WMS
        int main() override;

        std::shared_ptr<Workflow> workflow;
        const std::shared_ptr<BatchComputeService> batch_compute_service;
        const std::shared_ptr<StorageService> storage_service;

    };
}
#endif //WRENCH_EXAMPLE_TWO_TASKS_AT_A_TIME_BATCH_H
