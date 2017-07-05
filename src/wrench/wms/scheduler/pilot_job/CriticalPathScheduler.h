/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_CRITICALPATHSCHEDULER_H
#define WRENCH_CRITICALPATHSCHEDULER_H

#include <set>
#include <vector>

#include "wms/scheduler/pilot_job/PilotJobScheduler.h"

namespace wrench {

    /**
     * @brief A critical path pilot job Scheduler
     */
    class CriticalPathScheduler : public PilotJobScheduler {

    public:
        /***********************/
        /** \cond DEVELOPER    */
        /***********************/
        void schedule(Scheduler *, Workflow *, JobManager *, const std::set<ComputeService *> &);

    private:
        std::map<WorkflowTask *, double> flopsMap;

        double getFlops(Workflow *, const std::vector<WorkflowTask *>);

        unsigned long getMaxParallelization(Workflow *, std::set<WorkflowTask *>);
        /***********************/
        /** \endcond           */
        /***********************/
    };

}

#endif //WRENCH_CRITICALPATHSCHEDULER_H
