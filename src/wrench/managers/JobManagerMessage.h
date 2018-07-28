/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_JOBMANAGERMESSAGE_H
#define WRENCH_JOBMANAGERMESSAGE_H


#include <wrench/simulation/SimulationMessage.h>
#include <wrench/workflow/job/StandardJob.h>
#include <wrench-dev.h>

namespace wrench {


    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level class for messages received/sent by a JobManager
     */
    class JobManagerMessage : public SimulationMessage {
    protected:
        JobManagerMessage(std::string name);
    };

    class JobManagerStandardJobDoneMessage : public JobManagerMessage {
    public:
        JobManagerStandardJobDoneMessage(StandardJob *job, ComputeService *compute_service,
                                         std::map<WorkflowTask *, WorkflowTask::State> necessary_state_changes);


        StandardJob *job;
        ComputeService *compute_service;
        std::map<WorkflowTask *, WorkflowTask::State> necessary_state_changes;
    };

    class JobManagerStandardJobFailedMessage : public JobManagerMessage {
    public:
        JobManagerStandardJobFailedMessage(StandardJob *job, ComputeService *compute_service,
                                           std::map<WorkflowTask *, WorkflowTask::State> necessary_state_changes,
                                           std::set<WorkflowTask *> necessary_failure_count_increments,
                                           std::shared_ptr<FailureCause> cause);


        StandardJob *job;
        ComputeService *compute_service;
        std::map<WorkflowTask *, WorkflowTask::State> necessary_state_changes;
        std::set<WorkflowTask *> necessary_failure_count_increments;
        std::shared_ptr<FailureCause> cause;
    };

    /***********************/
    /** \endcond           */
    /***********************/

};

#endif //WRENCH_JOBMANAGERMESSAGE_H
