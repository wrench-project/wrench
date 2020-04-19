/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_ONE_TASK_AT_A_TIME_H
#define WRENCH_ONE_TASK_AT_A_TIME_H

#include <wrench-dev.h>


namespace wrench {

    class Simulation;

    /**
     *  @brief A Workflow Management System (WMS) implementation (inherits from WMS)
     */
    class OneTaskAtATimeWMS : public WMS {

    public:
        // Constructor
        OneTaskAtATimeWMS(
                  const std::set<std::shared_ptr<ComputeService>> &compute_services,
                  const std::set<std::shared_ptr<StorageService>> &storage_services,
                  const std::string &hostname);

    protected:

        // Overriden method
        void processEventStandardJobCompletion(std::shared_ptr<StandardJobCompletedEvent>) override;

    private:
        // main() method of the WMS
        int main() override;

    };
}
#endif //WRENCH_ONE_TASK_AT_A_TIME_H
