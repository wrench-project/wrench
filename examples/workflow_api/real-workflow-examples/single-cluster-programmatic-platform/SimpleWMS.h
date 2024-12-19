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

#include "wrench-dev.h"

namespace wrench {

    /**
     *  @brief A simple WMS implementation
     */
    class SimpleWMS : public ExecutionController {

    public:
        SimpleWMS(const std::shared_ptr<Workflow> &workflow,
                  const std::set<std::shared_ptr<wrench::BareMetalComputeService>> &bare_metal_compute_services,
                  const std::shared_ptr<StorageService> &storage_service,
                  const std::string &hostname);

    protected:
        void processEventStandardJobCompletion(const std::shared_ptr<StandardJobCompletedEvent> &event) override;
        void processEventStandardJobFailure(const std::shared_ptr<StandardJobFailedEvent> &event) override;

    private:
        int main() override;

        void scheduleReadyTasks(std::vector<std::shared_ptr<WorkflowTask>> ready_tasks);

        std::shared_ptr<Workflow> workflow;
        std::set<std::shared_ptr<wrench::BareMetalComputeService>> bare_metal_compute_services;
        std::shared_ptr<StorageService> storage_service;
        std::shared_ptr<JobManager> job_manager;

        std::map<std::shared_ptr<ComputeService>, unsigned long> core_utilization_map;
    };
}// namespace wrench

#endif//WRENCH_EXAMPLE_SIMPLEWMS_H
