/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_MULTICORETASKEXECUTOR_H
#define WRENCH_MULTICORETASKEXECUTOR_H


#include <queue>
#include "workflow/WorkflowTask.h"
#include "services/compute_services/ComputeService.h"
#include "helper_daemons/sequential_task_executor/SequentialTaskExecutor.h"
#include "simulation/SimulationMessage.h"
#include "MulticoreJobExecutorProperty.h"

namespace wrench {

    class Simulation;


    /**  @brief Implementation of a Compute Service abstraction that
     *   runs on a multi-core host.
     */
    class MulticoreJobExecutor : public ComputeService {

    public:
        

    private:

        std::map<std::string, std::string> default_property_values =
                {
                 {MulticoreJobExecutorProperty::STOP_DAEMON_MESSAGE_PAYLOAD,            "1024"},
                 {MulticoreJobExecutorProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD,         "1024"},
                 {MulticoreJobExecutorProperty::RUN_STANDARD_JOB_MESSAGE_PAYLOAD,       "1024"},
                 {MulticoreJobExecutorProperty::JOB_TYPE_NOT_SUPPORTED_MESSAGE_PAYLOAD, "1024"},
                 {MulticoreJobExecutorProperty::NOT_ENOUGH_CORES_MESSAGE_PAYLOAD,       "1024"},
                 {MulticoreJobExecutorProperty::STANDARD_JOB_DONE_MESSAGE_PAYLOAD,      "1024"},
                 {MulticoreJobExecutorProperty::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD,    "1024"},
                 {MulticoreJobExecutorProperty::RUN_PILOT_JOB_MESSAGE_PAYLOAD,          "1024"},
                 {MulticoreJobExecutorProperty::PILOT_JOB_STARTED_MESSAGE_PAYLOAD,      "1024"},
                 {MulticoreJobExecutorProperty::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD,      "1024"},
                 {MulticoreJobExecutorProperty::PILOT_JOB_FAILED_MESSAGE_PAYLOAD,       "1024"},
                 {MulticoreJobExecutorProperty::NUM_IDLE_CORES_REQUEST_MESSAGE_PAYLOAD, "1024"},
                 {MulticoreJobExecutorProperty::NUM_IDLE_CORES_ANSWER_MESSAGE_PAYLOAD,  "1024"},
                 {MulticoreJobExecutorProperty::TTL_REQUEST_MESSAGE_PAYLOAD,            "1024"},
                 {MulticoreJobExecutorProperty::TTL_ANSWER_MESSAGE_PAYLOAD,             "1024"},
                 {MulticoreJobExecutorProperty::TASK_STARTUP_OVERHEAD,                  "0.0"}
                };

    public:

        // Public Constructor
        MulticoreJobExecutor(std::string hostname,
                             bool supports_standard_jobs,
                             bool supports_pilot_jobs,
                             std::map<std::string, std::string> = {});

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/



        // Running jobs
        void runStandardJob(StandardJob *job);

        void runPilotJob(PilotJob *job);

        // Getting information
        unsigned long getNumCores();

        unsigned long getNumIdleCores();

        double getTTL();

        double getCoreFlopRate();


        /***********************/
        /** \endcond           */
        /***********************/

    private:

        friend class Simulation;

        // Low-level Constructor
        MulticoreJobExecutor(std::string hostname,
                             bool supports_standard_jobs,
                             bool supports_pilot_jobs,
                             std::map<std::string, std::string>,
                             unsigned int num_worker_threads,
                             double ttl,
                             PilotJob *pj, std::string suffix);

        std::string hostname;
        unsigned int num_worker_threads; // total threads to run tasks from standard jobs
        double ttl;
        bool has_ttl;
        double death_date;
        PilotJob *containing_pilot_job;

        unsigned int num_available_worker_threads; // number of worker threads that can currently be
        // used to run tasks from standard jobs

        // Vector of worker threads
        std::vector<std::unique_ptr<SequentialTaskExecutor>> sequential_task_executors;

        // Vector of idle worker threads available to run tasks from standard jobs
        std::set<SequentialTaskExecutor *> idle_sequential_task_executors;
        // Vector of worker threads currencly running tasks from standard jobs
        std::set<SequentialTaskExecutor *> busy_sequential_task_executors;

        // Queue of pending jobs (standard or pilot) that haven't began executing
        std::queue<WorkflowJob *> pending_jobs;

        // Set of currently running (standard or pilot) jobs
        std::set<WorkflowJob *> running_jobs;

        // Queue of standard job tasks waiting for execution
        std::queue<WorkflowTask *> pending_tasks;

        // Set of currently running standard job tasks
        std::set<WorkflowTask *> running_tasks;

        int main();

        // Helper functions to make main() a bit more palatable
        void initialize();

        void terminate();

        void terminateAllWorkerThreads();

        void terminateAllPilotJobs();

        void failCurrentStandardJobs();

        void processTaskCompletion(WorkflowTask *, SequentialTaskExecutor *);

        void processPilotJobCompletion(PilotJob *job);

        bool processNextMessage(double timeout);

        bool dispatchNextPendingTask();

        bool dispatchNextPendingJob();
    };
};


#endif //WRENCH_MULTICORETASKEXECUTOR_H
