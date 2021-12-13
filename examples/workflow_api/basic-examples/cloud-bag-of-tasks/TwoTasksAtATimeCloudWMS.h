/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_EXAMPLE_TWO_TASKS_AT_A_TIME_CLOUD_H
#define WRENCH_EXAMPLE_TWO_TASKS_AT_A_TIME_CLOUD_H

#include <wrench-dev.h>


namespace wrench {

    class Simulation;

    /**
     *  @brief A Workflow Management System (WMS) implementation (inherits from WMS)
     */
    class TwoTasksAtATimeCloudWMS : public WMS {

    public:
        // Constructor
        TwoTasksAtATimeCloudWMS(
                std::shared_ptr<Workflow> workflow,
                const std::set<std::shared_ptr<ComputeService>> &compute_services,
                const std::set<std::shared_ptr<StorageService>> &storage_services,
                const std::string &hostname);

    protected:

        // Overriden method
        void processEventStandardJobCompletion(std::shared_ptr<StandardJobCompletedEvent>) override;
        void processEventStandardJobFailure(std::shared_ptr<StandardJobFailedEvent>) override;

    private:
        // main() method of the WMS
        int main() override;

    };
}
#endif //WRENCH_EXAMPLE_TWO_TASKS_AT_A_TIME_CLOUD_H
