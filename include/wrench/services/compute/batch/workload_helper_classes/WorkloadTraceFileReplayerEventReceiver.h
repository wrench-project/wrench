/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_ONEJOBWMS_H
#define WRENCH_ONEJOBWMS_H

#include "wrench/execution_controller/ExecutionController.h"

/***********************/
/** \cond INTERNAL    **/
/***********************/

namespace wrench {

    class BatchComputeService;

    /**
     * @brief A WMS that only submits a single job to a given batch service, which is used
     *        to implement batch workload replay
     */
    class WorkloadTraceFileReplayerEventReceiver : public ExecutionController {

    public:
        /**
         * @brief Constructor
         * @param hostname: the name of the host on which the "one job" WMS will run
         * @param job_manager: A JobManager with which to interact
         */
        WorkloadTraceFileReplayerEventReceiver(std::string hostname, std::shared_ptr<JobManager> job_manager) : ExecutionController(
                                                                                                                        hostname,
                                                                                                                        "workload_trace_file_replayer_event_receiver"),
                                                                                                                job_manager(job_manager) {}

        int main() override;

    private:
        std::string hostname;
        std::shared_ptr<JobManager> job_manager;
    };

};// namespace wrench

    /***********************/
    /** \endcond          **/
    /***********************/

#endif//WRENCH_ONEJOBWMS_H
