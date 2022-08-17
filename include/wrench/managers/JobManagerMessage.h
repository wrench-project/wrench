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

#include "wrench/simulation/SimulationMessage.h"
#include "wrench/services/compute/ComputeService.h"
#include "wrench/job/StandardJob.h"
#include "wrench-dev.h"

namespace wrench {


    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level class for messages received/sent by a JobManager
     */
    class JobManagerMessage : public SimulationMessage {
    protected:
        explicit JobManagerMessage();
    };

    /**
   * @brief A message sent to a JobManager to wake it up
   */
    class JobManagerWakeupMessage : public JobManagerMessage {
    public:
        JobManagerWakeupMessage();
    };

    /**
    * @brief A message sent by the JobManager to notify some submitter that a CompoundJob has completed
    */
    class JobManagerCompoundJobCompletedMessage : public JobManagerMessage {
    public:
        JobManagerCompoundJobCompletedMessage(std::shared_ptr<CompoundJob> job, std::shared_ptr<ComputeService> compute_service);

        /** @brief The job that is done */
        std::shared_ptr<CompoundJob> job;
        /** @brief The compute service on which the job ran */
        std::shared_ptr<ComputeService> compute_service;
    };

    /**
     * @brief A message sent by the JobManager to notify some submitter that a CompoundJob has failed
     */
    class JobManagerCompoundJobFailedMessage : public JobManagerMessage {
    public:
        JobManagerCompoundJobFailedMessage(std::shared_ptr<CompoundJob> job,
                                           std::shared_ptr<ComputeService> compute_service,
                                           std::shared_ptr<FailureCause> cause);


        /** @brief The job that has failed */
        std::shared_ptr<CompoundJob> job;
        /** @brief The compute service on which the job has failed */
        std::shared_ptr<ComputeService> compute_service;
        /** @brief The failure cause */
        std::shared_ptr<FailureCause> cause;
    };


    /**
     * @brief A message sent by the JobManager to notify some submitter that a StandardJob has completed
     */
    class JobManagerStandardJobCompletedMessage : public JobManagerMessage {
    public:
        JobManagerStandardJobCompletedMessage(std::shared_ptr<StandardJob> job, std::shared_ptr<ComputeService> compute_service,
                                              std::map<std::shared_ptr<WorkflowTask>, WorkflowTask::State> necessary_state_changes);


        /** @brief The job that is done */
        std::shared_ptr<StandardJob> job;
        /** @brief The compute service on which the job ran */
        std::shared_ptr<ComputeService> compute_service;
        /** @brief The necessary task1 state changes */
        std::map<std::shared_ptr<WorkflowTask>, WorkflowTask::State> necessary_state_changes;
    };

    /**
     * @brief A message sent by the JobManager to notify some submitter that a StandardJob has failed
     */
    class JobManagerStandardJobFailedMessage : public JobManagerMessage {
    public:
        JobManagerStandardJobFailedMessage(std::shared_ptr<StandardJob> job, std::shared_ptr<ComputeService> compute_service,
                                           std::map<std::shared_ptr<WorkflowTask>, WorkflowTask::State> necessary_state_changes,
                                           std::set<std::shared_ptr<WorkflowTask>> necessary_failure_count_increments,
                                           std::shared_ptr<FailureCause> cause);


        /** @brief The job that has failed */
        std::shared_ptr<StandardJob> job;
        /** @brief The compute service on which the job has failed */
        std::shared_ptr<ComputeService> compute_service;
        /** @brief The task1 state change that should be made */
        std::map<std::shared_ptr<WorkflowTask>, WorkflowTask::State> necessary_state_changes;
        /** @brief The tasks whose failure counts need to be incremented */
        std::set<std::shared_ptr<WorkflowTask>> necessary_failure_count_increments;
        /** @brief The cause of the failure */
        std::shared_ptr<FailureCause> cause;
    };

//    /**
//    * @brief A message sent by the JobManager to notify some submitter that a PilotJob has failed
//    */
//    class JobManagerPilotJobFailedMessage : public JobManagerMessage {
//    public:
//        JobManagerPilotJobFailedMessage(std::shared_ptr<PilotJob> job,
//                                        std::shared_ptr<ComputeService> compute_service,
//                                        std::shared_ptr<FailureCause> cause);
//
//
//        /** @brief The job that has failed */
//        std::shared_ptr<PilotJob> job;
//        /** @brief The compute service on which the job has failed */
//        std::shared_ptr<ComputeService> compute_service;
//        /** @brief The failure cause */
//        std::shared_ptr<FailureCause> cause;
//    };

    /***********************/
    /** \endcond           */
    /***********************/

};// namespace wrench

#endif//WRENCH_JOBMANAGERMESSAGE_H
