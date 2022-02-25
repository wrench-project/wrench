/**
 * Copyright (c) 2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_CONDORWMS_H
#define WRENCH_CONDORWMS_H

#include <wrench-dev.h>


namespace wrench {

    /**
     *  @brief A Workflow Management System (WMS) implementation
     */
    class CondorWMS : public ExecutionController {

    public:
        // Constructor
        CondorWMS(const std::shared_ptr<Workflow> &workflow,
                  const std::shared_ptr<wrench::HTCondorComputeService> &htcondor_compute_service,
                  const std::shared_ptr<wrench::BatchComputeService> &batch_compute_service,
                  const std::shared_ptr<wrench::CloudComputeService> &cloud_compute_service,
                  const std::shared_ptr<wrench::StorageService> &storage_service,
                  std::string hostname);

    protected:

        // Overridden method
        void processEventStandardJobCompletion(std::shared_ptr<StandardJobCompletedEvent>) override;
        /**
        void processEventStandardJobFailure(std::shared_ptr<StandardJobFailedEvent>) override;
        **/
    private:
        // main() method of the WMS
        int main() override;

        std::shared_ptr<Workflow> workflow;
        std::shared_ptr<wrench::HTCondorComputeService> htcondor_compute_service;
        std::shared_ptr<wrench::BatchComputeService> batch_compute_service;
        std::shared_ptr<wrench::CloudComputeService> cloud_compute_service;
        std::shared_ptr<wrench::StorageService> storage_service;
    };

}

#endif //WRENCH_CONDORWMS_H
