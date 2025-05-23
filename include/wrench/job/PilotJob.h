/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_PILOTJOB_H
#define WRENCH_PILOTJOB_H

#include "Job.h"

namespace wrench {


    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    class BareMetalComputeService;

    /**
     * @brief A pilot (i.e., non-standard) workflow job that can be submitted to a ComputeService
     * by a WMS (via a JobManager)
     */
    class PilotJob : public Job {

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

        std::shared_ptr<BareMetalComputeService> getComputeService();

        PilotJob::State getState() const;

        /***********************/
        /** \cond INTERNAL     */
        /***********************/


        //        void setComputeService(std::shared_ptr<BareMetalComputeService> cs);

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        std::shared_ptr<CompoundJob> compound_job;

        friend class JobManager;

        explicit PilotJob(std::shared_ptr<JobManager> job_manager);

        State state;
        std::shared_ptr<BareMetalComputeService> compute_service;// Associated compute service, i.e., the running pilot job
    };

    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench


#endif//WRENCH_PILOTJOB_H
