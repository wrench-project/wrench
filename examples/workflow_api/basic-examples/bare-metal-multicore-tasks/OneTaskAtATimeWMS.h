/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_EXAMPLE_ONE_TASK_AT_A_TIME_H
#define WRENCH_EXAMPLE_ONE_TASK_AT_A_TIME_H

#include <wrench-dev.h>


namespace wrench {

    /**
     *  @brief A Workflow Management System (WMS) implementation
     */
    class OneTaskAtATimeWMS : public ExecutionController {

    public:
        // Constructor
        OneTaskAtATimeWMS(
                const std::shared_ptr<Workflow> &workflow,
                const std::shared_ptr<BareMetalComputeService> &bare_metal_compute_service,
                const std::string &hostname);

    protected:
        // Overridden method
        void processEventStandardJobCompletion(const std::shared_ptr<StandardJobCompletedEvent> &event) override;
        void processEventStandardJobFailure(const std::shared_ptr<StandardJobFailedEvent> &event) override;

    private:
        // main() method of the WMS
        int main() override;

        std::shared_ptr<Workflow> workflow;
        std::shared_ptr<BareMetalComputeService> bare_metal_compute_service;
    };
}// namespace wrench
#endif//WRENCH_EXAMPLE_ONE_TASK_AT_A_TIME_H
