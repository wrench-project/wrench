/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_BATCHSERVICEPROPERTY_H
#define WRENCH_BATCHSERVICEPROPERTY_H

#include "wrench/services/compute/ComputeServiceProperty.h"

namespace wrench {


    /**
     * @brief Properties for a BatchService
     */
    class BatchServiceProperty: public ComputeServiceProperty {

    public:
        /**
         * @brief The overhead to start a thread execution, in seconds
         */
        DECLARE_PROPERTY_NAME(THREAD_STARTUP_OVERHEAD);

        /**
         * @brief The batch scheduling algorithm. Can be:
         *    - If ENABLE_BATSCHED is set to off / not set:
         *      - "FCFS": First Come First Serve
         *    - If ENABLE_BATSCHED is set to on:
         *      - whatever scheduling algorithm is supported by Batsched
         *                  (by default: "easy_bf")
         *
         **/
        DECLARE_PROPERTY_NAME(BATCH_SCHEDULING_ALGORITHM);

        /**
         * @brief The batch queue ordering algorithm
         *     - If ENABLE_BATSCHED is set to off / not set: ignored
         *     - If ENABLE_BATSCHED is set to on:
         *       - whatever queue ordering algorithm is supported by Batsched
         *                  (by default: "fcfs")
         */
        DECLARE_PROPERTY_NAME(BATCH_QUEUE_ORDERING_ALGORITHM);

        /**
         * @brief The host selection algorithm (only used if
         *         ENABLE_BATSCHED is set of off or not set). Can be:
         *    - FIRSTFIT
         *    - BESTFIT
         **/
        DECLARE_PROPERTY_NAME(HOST_SELECTION_ALGORITHM);

        /**
         * @brief Path to a workload trace file to be replayed. The trace file neede to
         * be in the SWF format (see http://www.cs.huji.ac.il/labs/parallel/workload/swf.html).
         * Note that jobs in the trace whose node processor requirements exceed the capacity
         * of the batch service will simply be capped at that capacity.
         */
        DECLARE_PROPERTY_NAME(SIMULATED_WORKLOAD_TRACE_FILE);


        /**
         * @brief Number of seconds that the Batch Scheduler adds to each incoming
         *        (standard) job. This is something production batch systems do (e.g.,
         *        if I say that I want to run a job that lasts at most 60 seconds, the system
         *        will actually assume it runs at most 65 seconds).
         */
        DECLARE_PROPERTY_NAME(BATCH_RJMS_DELAY);

        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        /**
         * @brief the number of bytes in the batsched "ready" message (probably should
         *        always be set to 0 as we don't really want to simulate the WRENCH-Batsched
         *        interaction delay)
         */
        DECLARE_PROPERTY_NAME(BATCH_SCHED_READY_PAYLOAD);
        /**
         * @brief the number of bytes in the batsched "execute job" message (probably should
         *        always be set to 0 as we don't really want to simulate the WRENCH-Batsched
         *        interaction delay)
         */
        DECLARE_PROPERTY_NAME(BATCH_EXECUTE_JOB_PAYLOAD);

        /***********************/
        /** \endcond           */
        /***********************/

    };
}


#endif //WRENCH_BATCHSERVICEPROPERTY_H
