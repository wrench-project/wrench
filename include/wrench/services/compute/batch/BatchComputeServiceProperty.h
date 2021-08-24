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
     * @brief Configurable properties for a BatchComputeService
     */
    class BatchComputeServiceProperty: public ComputeServiceProperty {

    public:
        /**
         * @brief The overhead to start a task execution, in seconds
         */
        DECLARE_PROPERTY_NAME(TASK_STARTUP_OVERHEAD);

        /**
         * @brief The batch scheduling algorithm. Can be:
         *    - If ENABLE_BATSCHED is set to off / not set:
         *      - "fcfs": First Come First Serve (default)
         *      - "conservative_bf": a home-grown implementation of FCFS with conservative backfilling, which only allocates resources at the node level (i.e., two jobs can never run on the same node even if that node has enough cores to support both jobs)
         *      - "conservative_bf_core_level": a home-grown implementation of FCFS with conservative backfilling, which only allocates resources at the core level (i.e., two jobs may run on the same node  if that node has enough cores to support both jobs)
         *
         *    - If ENABLE_BATSCHED is set to on:
         *      - whatever scheduling algorithm is supported by Batsched
         *        (by default: "conservative_bf", other options include
         *        "easy_bf" and "easy_bf_fast")
         *      - These only allocate resources at the node level (i.e., two jobs can never run on the same node even if that node has enough cores to support both jobs)
         *
         **/
        DECLARE_PROPERTY_NAME(BATCH_SCHEDULING_ALGORITHM);

        /**
         * @brief The batch queue ordering algorithm. Can be:
         *     - If ENABLE_BATSCHED is set to off / not set: ignored
         *     - If ENABLE_BATSCHED is set to on:
         *       - whatever queue ordering algorithm is supported by Batsched
         *                  (by default: "fcfs")
         */
        DECLARE_PROPERTY_NAME(BATCH_QUEUE_ORDERING_ALGORITHM);

        /**
         * @brief The host selection algorithm. Can be:
         *      - If ENABLE_BATSCHED is set to on or if the BATCH_SCHEDULING_ALGORITHM is not fcfs: ignored
         *      - If ENABLE_BATSCHED is set to off or not set, and if the BATCH_SCHEDULING_ALGORITHM is fcfs:
         *          - FIRSTFIT  (default)
         *          - BESTFIT
         *          - ROUNDROBIN
         **/
        DECLARE_PROPERTY_NAME(HOST_SELECTION_ALGORITHM);

        /** @brief The algorithm to pick which ready computational task (within a standard job
         *         executed by the batch service), in case multiple tasks are ready, should run first. This is typically
         *         not managed by a batch scheduler, but by some application-level script that executes
         *         a set of tasks within compute resources allocated by the batch scheduler. Possible values are:
         *                  - maximum_flops (default)
         *                  - maximum_minimum_cores
         *                  - minimum_top_level
         **/
        DECLARE_PROPERTY_NAME(TASK_SELECTION_ALGORITHM);

        /**
         * @brief Path to a workload trace file to be replayed. The trace file can be
         * be in the SWF format (see http://www.cs.huji.ac.il/labs/parallel/workload/swf.html), in which
         * case it must have extension ".swf", or in the JSON format as used in the BATSIM project
         * (see https://github.com/oar-team/batsim), in which case is must have the ".json" extension).
         * The jobs in the trace whose node/host/processor/core requirements exceed the capacity
         * of the batch service will simply be capped at that capacity. Job submission times in the trace files
         * are relative to the batch's start time (i.e., all jobs in the trace files will be replayed
         * assuming that the batch starts at time zero). Note that in the BATSIM JSON format, the trace does not
         * contains requested vs. actual trace runtimes, and to all requested runtimes are 100% accurate.
         */
        DECLARE_PROPERTY_NAME(SIMULATED_WORKLOAD_TRACE_FILE);

        /**
         * @brief Whether, when simulating a workload trace file, to use the actual runtimes
         * as requested runtimes (i.e., simulating users who request exactly what they need)
         * or not (i.e., simulating users who always overestimate what they need, which is
         * typical in the real world):
         *      - "true": use real runtimes as requested runtimes
         *      - "false": use requested times from the trace file
         */
        DECLARE_PROPERTY_NAME(USE_REAL_RUNTIMES_AS_REQUESTED_RUNTIMES_IN_WORKLOAD_TRACE_FILE);

        /**
         * @brief Whether, when simulating a workload trace file, to abort when there
         * is an invalid job specification (e.g., negative times, negative allocations),
         * or to simply print a warning.
         *      - "true": merely print a warning whenever there is an invalid job
         *      - "false": abort whenever there is an invalid job
         */
        DECLARE_PROPERTY_NAME(IGNORE_INVALID_JOBS_IN_WORKLOAD_TRACE_FILE);

        /**
         * @brief A specification of the submit time of the first job in a provided trace file.
         *          - A positive number: the submit time of the first job
         *          - A strictly negative number: use whatever submit time is in the trace file
         */
        DECLARE_PROPERTY_NAME(SUBMIT_TIME_OF_FIRST_JOB_IN_WORKLOAD_TRACE_FILE);

        /**
         * @brief Path to a to-be-generated Batsim-style CSV trace file (e.g. for b3atch schedule visualization purposes).
         *      - If ENABLE_BATSCHED is set to off or not set: ignored
         *      - If ENABLE_BATSCHED is set to on: The trace file is generated in CSV format as follows:
         *          allocated_processors,consumed_energy,execution_time,finish_time,job_id,metadata,
         *          requested_number_of_processors,requested_time,starting_time,stretch,submission_time,success,
         *          turnaround_time,waiting_time,workload_name

         */
        DECLARE_PROPERTY_NAME(OUTPUT_CSV_JOB_LOG);


        /**
         * @brief Integral number of seconds that the Batch Scheduler adds to the runtime of each incoming
         *        job. This is something production batch systems do to avoid too aggressive job
         *        terminations. For instance,
         *        if a job says it wants to run for (at most) 60 seconds, the system
         *        will actually assume the job wants to run for (at most) 60 + 5 seconds.
         */
        DECLARE_PROPERTY_NAME(BATCH_RJMS_PADDING_DELAY);

        /** @brief Simulate computation as just a sleep instead of an actual compute thread. This is for scalability reason,
         *        and only simulation-valid
        *         if one is sure that cores are space shared (i.e., only a single compute thread can ever
        *         run on a core at once). Since space-sharing at the core level is typically the case in batch-scheduled
         *        clusters, this is likely fine.
         *           - "true": simulate computation as sleep
         *           - "false": do not simulate computation as sleep (default)
        */
        DECLARE_PROPERTY_NAME(SIMULATE_COMPUTATION_AS_SLEEP);

        /** @brief Controls Batsched logging
         *      - If ENABLE_BATSCHED is set to off or not set: ignored
         *      - If ENABLE_BATSCHED is set to on:
         *          - "true": do not show Batsched logging output on the terminal (default)
         *          - "false": show Batsched logging output on the terminal
         */
        DECLARE_PROPERTY_NAME(BATSCHED_LOGGING_MUTED);

        /** @brief Controls Batsched node allocation policy
        *      - If ENABLE_BATSCHED is set to off or not set: ignored
        *      - If ENABLE_BATSCHED is set to on:
        *          - "false": do not enforce contiguous nodes for allocations (default)
        *          - "true": enforce contiguous nodes for allocations (note that not all algorithms
        *            implemented by batsched support contiguous allocations, so this option may
        *            have no effect in some cases).
        */
        DECLARE_PROPERTY_NAME(BATSCHED_CONTIGUOUS_ALLOCATION);

//        /** @brief Overhead delay in seconds between condor and slurm for the start of execution
//         *      - defaults to calibrated figure
//         *      - property is set on first receiving grid universe job.
//         */
//        DECLARE_PROPERTY_NAME(GRID_PRE_EXECUTION_DELAY);
//
//        /** @brief Overhead delay in seconds between condor and slurm for the completion of execution
//         *      - defaults to calibrated figure
//         *      - property is set on first receiving grid universe job.
//         */
//        DECLARE_PROPERTY_NAME(GRID_POST_EXECUTION_DELAY);



        /***********************/
        /** \cond INTERNAL     */
        /***********************/


        /***********************/
        /** \endcond           */
        /***********************/


    };
}


#endif //WRENCH_BATCHSERVICEPROPERTY_H
