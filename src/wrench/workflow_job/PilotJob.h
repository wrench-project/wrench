/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_PILOTJOB_H
#define WRENCH_PILOTJOB_H


#include "workflow_job/WorkflowJob.h"

namespace wrench {

    class ComputeService;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief A pilot WorkflowJob
     */
    class PilotJob : public WorkflowJob {

    public:

        /** @brief Pilot job states */
        enum State {
            /** @brief Not submitted yet */
            NOT_SUBMITTED,
            /** @brief Submitted but not running */
            PENDING,
            /** @brief Running */
            RUNNING,
            /** @brief Expired due to a time-to-live limit */
            EXPIRED,
            /** @brief Failed */
            FAILED
        };

        ComputeService *getComputeService();

        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        void setComputeService(ComputeService *);

        /***********************/
        /** \endcond           */
        /***********************/

    private:

        friend class JobManager;

        PilotJob(Workflow *workflow, unsigned long num_cores, double duration);

        PilotJob::State getState();

        State state;
        ComputeService *compute_service; // Associated compute service, i.e., the running pilot job
    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_PILOTJOB_H
