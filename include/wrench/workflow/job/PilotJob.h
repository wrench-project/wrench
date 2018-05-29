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


#include "wrench/workflow/job/WorkflowJob.h"
#include "wrench/services/compute/ComputeService.h"

namespace wrench {

    class ComputeService;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief A pilot (i.e., non-standard) workflow job that can be submitted to a ComputeService
     * by a WMS (via a JobManager)
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
            FAILED,
            /** @brief Terminated by submitter **/
            TERMINATED
        };

        ComputeService *getComputeService();

        PilotJob::State getState();

        unsigned long getNumHosts() const;

        unsigned long getNumCoresPerHost() const;

        double getMemoryPerHost() const;

        double getDuration() const;



        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        void setComputeService(std::shared_ptr<ComputeService> cs);


        /***********************/
        /** \endcond           */
        /***********************/

    private:

        friend class JobManager;

        PilotJob(Workflow *workflow, unsigned long num_hosts, unsigned long num_cores_per_host, double ram_per_host, double duration);

        unsigned long num_hosts;
        unsigned long num_cores_per_host;
        double ram_per_host;
        double duration;


        State state;
        std::shared_ptr<ComputeService> compute_service; // Associated compute service, i.e., the running pilot job
    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_PILOTJOB_H
