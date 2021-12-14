/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_EXAMPLE_COMPLEX_JOB_H
#define WRENCH_EXAMPLE_COMPLEX_JOB_H

#include <wrench-dev.h>


namespace wrench {

    class Simulation;

    /**
     *  @brief A Workflow Management System (WMS) implementation
     */
    class ComplexJobWMS : public ExecutionController {

    public:
        // Constructor
        ComplexJobWMS(
                const std::shared_ptr<Workflow> &workflow,
                const std::shared_ptr<ComputeService> &bare_metal_compute_service,
                const std::shared_ptr<StorageService> &storage_service1,
                const std::shared_ptr<StorageService> &storage_service2,
                const std::string &hostname);

    protected:

        // Overridden method
        void processEventStandardJobCompletion(std::shared_ptr<StandardJobCompletedEvent>) override;
        void processEventStandardJobFailure(std::shared_ptr<StandardJobFailedEvent>) override;

    private:
        // main() method of the WMS
        int main() override;

        std::shared_ptr<Workflow> workflow;
        std::shared_ptr<ComputeService> bare_metal_compute_service;
        std::shared_ptr<StorageService> storage_service1;
        std::shared_ptr<StorageService> storage_service2;


    };
}
#endif //WRENCH_EXAMPLE_COMPLEX_JOB_H
