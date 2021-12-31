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

        void processEventStandardJobCompletion(std::shared_ptr<StandardJobCompletedEvent> event) override;
        void processEventStandardJobFailure(std::shared_ptr<StandardJobFailedEvent> event) override;
        void processEventPilotJobStart(std::shared_ptr<PilotJobStartedEvent> event) override;
        void processEventPilotJobExpiration(std::shared_ptr<PilotJobExpiredEvent> event) override;

    private:
        int main() override;

        /** @brief Whether the workflow execution should be aborted */
        bool abort = false;

        /** @brief A pilot job that is submitted to the batch compute service */
        std::shared_ptr<PilotJob> pilot_job = nullptr;
        /** @brief A boolean to indicate whether the pilot job is running */
        bool pilot_job_is_running = false;

        void scheduleReadyTasks(std::vector<std::shared_ptr<WorkflowTask>> ready_tasks,
                                std::shared_ptr<JobManager> job_manager,
                                std::set<std::shared_ptr<BareMetalComputeService>> compute_services);

        std::shared_ptr<Workflow> workflow;
        std::shared_ptr<BatchComputeService> batch_compute_service;
        std::shared_ptr<CloudComputeService> cloud_compute_service;
        std::shared_ptr<StorageService> storage_service;

        std::map<std::shared_ptr<ComputeService>, unsigned long> core_utilization_map;

    };
}
#endif //WRENCH_EXAMPLE_SIMPLEWMS_H
