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
    class SimpleWMS : public WMS {

    public:
        SimpleWMS(std::unique_ptr<StandardJobScheduler> standard_job_scheduler,
                  std::unique_ptr<PilotJobScheduler> pilot_job_scheduler,
                  const std::set<std::shared_ptr<ComputeService>> &compute_services,
                  const std::set<std::shared_ptr<StorageService>> &storage_services,
                  const std::string &hostname);

    protected:

        void processEventStandardJobFailure(std::shared_ptr<StandardJobFailedEvent>) override;

    private:
        int main() override;

        /** @brief The job manager */
        std::shared_ptr<JobManager> job_manager;
        /** @brief Whether the workflow execution should be aborted */
        bool abort = false;
    };
}
#endif //WRENCH_EXAMPLE_SIMPLEWMS_H
