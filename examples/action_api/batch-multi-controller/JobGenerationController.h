/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_EXAMPLE_JOB_GENERATION_CONTROLLER_H
#define WRENCH_EXAMPLE_JOB_GENERATION_CONTROLLER_H

#include <wrench-dev.h>

namespace wrench {

    class BatchServiceController;

    /**
     *  @brief An execution controller implementation
     */
    class JobGenerationController : public ExecutionController {

    public:
        // Constructor
        JobGenerationController(
                const std::string &hostname,
                int num_jobs,
                const std::vector<std::shared_ptr<BatchServiceController>> &batch_service_controllers);

    private:
        int main() override;

        int _num_jobs;
        std::vector<std::shared_ptr<BatchServiceController>> _batch_service_controllers;

        void processEventCustom(const std::shared_ptr<CustomEvent> &event) override;


    };

}// namespace wrench
#endif//WRENCH_EXAMPLE_JOB_GENERATION_CONTROLLER_H
