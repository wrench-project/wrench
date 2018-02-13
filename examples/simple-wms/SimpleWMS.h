/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SIMPLEWMS_H
#define WRENCH_SIMPLEWMS_H

#include <wrench-dev.h>

namespace wrench {

    class Simulation;

    /**
     *  @brief A simple WMS implementation
     */
    class SimpleWMS : public WMS {

    public:
        SimpleWMS(Workflow *,
                  std::unique_ptr<Scheduler>,
                  const std::set<ComputeService *> &compute_services,
                  const std::string);

    protected:
        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        void processEventStandardJobFailure(std::unique_ptr<WorkflowExecutionEvent>);

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        int main();

        /** @brief The job manager */
        std::unique_ptr<JobManager> job_manager;
        /** @brief Whether the workflow execution should be aborted */
        bool abort = false;
    };
}
#endif //WRENCH_SIMPLEWMS_H
