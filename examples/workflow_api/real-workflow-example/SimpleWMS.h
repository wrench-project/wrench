/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_EXAMPLE_SIMPLEWMS_H
#define WRENCH_EXAMPLE_SIMPLEWMS_H

#include <wrench-dev.h>

namespace wrench {

    class Simulation;

    /**
     *  @brief A simple WMS implementation
     */
    class SimpleWMS : public ExecutionController {

    public:
        SimpleWMS(const std::shared_ptr<Workflow> &workflow,
                  const std::shared_ptr<BatchComputeService> &batch_compute_service,
                  const std::shared_ptr<CloudComputeService> &cloud_compute_service,
                  const std::shared_ptr<StorageService> &storage_service,
                  const std::string &hostname);

    protected:

        void processEventStandardJobFailure(std::shared_ptr<StandardJobFailedEvent>) override;

    private:
        int main() override;

        /** @brief Whether the workflow execution should be aborted */
        bool abort = false;

        bool scheduleTask(std::shared_ptr<JobManager> job_manager,
                          std::shared_ptr<WorkflowTask> task,
                          std::set<std::shared_ptr<BareMetalComputeService>> compute_services);

        std::shared_ptr<Workflow> workflow;
        std::shared_ptr<CloudComputeService> cloud_compute_service;
        std::shared_ptr<BatchComputeService> batch_compute_service;
        std::shared_ptr<StorageService> storage_service;

    };
}
#endif //WRENCH_EXAMPLE_SIMPLEWMS_H
