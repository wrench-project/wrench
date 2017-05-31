/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_MULTICORECOMPUTESERVICE_H
#define WRENCH_MULTICORECOMPUTESERVICE_H


#include <queue>

#include <services/compute_services/ComputeService.h>

#include "MulticoreComputeServiceProperty.h"
#include "WorkerThread.h"

namespace wrench {

    class Simulation;

    class StorageService;

    class WorkflowExecutionFailureCause;

    /**  @brief Implementation of a Compute Service abstraction that
     *   runs on a multi-core host.
     */
    class MulticoreComputeService : public ComputeService {

    public:


    private:

        std::map<std::string, std::string> default_property_values =
                {
                        {MulticoreComputeServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD,                 "1024"},
                        {MulticoreComputeServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD,              "1024"},
                        {MulticoreComputeServiceProperty::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD, "1024"},
                        {MulticoreComputeServiceProperty::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD,  "1024"},
                        {MulticoreComputeServiceProperty::JOB_TYPE_NOT_SUPPORTED_MESSAGE_PAYLOAD,      "1024"},
                        {MulticoreComputeServiceProperty::NOT_ENOUGH_CORES_MESSAGE_PAYLOAD,            "1024"},
                        {MulticoreComputeServiceProperty::STANDARD_JOB_DONE_MESSAGE_PAYLOAD,           "1024"},
                        {MulticoreComputeServiceProperty::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD,         "1024"},
                        {MulticoreComputeServiceProperty::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD,    "1024"},
                        {MulticoreComputeServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD,     "1024"},
                        {MulticoreComputeServiceProperty::PILOT_JOB_STARTED_MESSAGE_PAYLOAD,           "1024"},
                        {MulticoreComputeServiceProperty::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD,           "1024"},
                        {MulticoreComputeServiceProperty::PILOT_JOB_FAILED_MESSAGE_PAYLOAD,            "1024"},
                        {MulticoreComputeServiceProperty::NUM_IDLE_CORES_REQUEST_MESSAGE_PAYLOAD,      "1024"},
                        {MulticoreComputeServiceProperty::NUM_IDLE_CORES_ANSWER_MESSAGE_PAYLOAD,       "1024"},
                        {MulticoreComputeServiceProperty::TTL_REQUEST_MESSAGE_PAYLOAD,                 "1024"},
                        {MulticoreComputeServiceProperty::TTL_ANSWER_MESSAGE_PAYLOAD,                  "1024"},
                        {MulticoreComputeServiceProperty::FLOP_RATE_REQUEST_MESSAGE_PAYLOAD,           "1024"},
                        {MulticoreComputeServiceProperty::FLOP_RATE_ANSWER_MESSAGE_PAYLOAD,            "1024"},
                        {MulticoreComputeServiceProperty::TASK_STARTUP_OVERHEAD,                       "0.0"}
                };

    public:

        // Public Constructor
        MulticoreComputeService(std::string hostname,
                             bool supports_standard_jobs,
                             bool supports_pilot_jobs,
                             std::map<std::string, std::string> = {});

        // Public Constructor
        MulticoreComputeService(std::string hostname,
                             bool supports_standard_jobs,
                             bool supports_pilot_jobs,
                             StorageService *default_storage_service,
                             std::map<std::string, std::string> = {});



        /***********************/
        /** \cond DEVELOPER    */
        /***********************/



        // Running jobs
        void submitStandardJob(StandardJob *job);

        void submitPilotJob(PilotJob *job);

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
        MulticoreComputeService(std::string hostname,
                             bool supports_standard_jobs,
                             bool supports_pilot_jobs,
                             std::map<std::string, std::string>,
                             unsigned int num_worker_threads,
                             double ttl,
                             PilotJob *pj, std::string suffix,
                             StorageService *default_storage_service);

        std::string hostname;
        unsigned int num_worker_threads; // total threads to run tasks from standard jobs
        double ttl;
        bool has_ttl;
        double death_date;
        PilotJob *containing_pilot_job;

        unsigned int num_available_worker_threads; // number of worker threads that can currently be
        // used to run tasks from standard jobs

        // Vector of worker threads
        std::vector<std::unique_ptr<WorkerThread>> worker_threads;

        // Vector of idle worker threads available to run tasks from standard jobs
        std::set<WorkerThread *> idle_worker_threads;
        // Vector of worker threads currently running tasks from standard jobs
        std::set<WorkerThread *> busy_worker_threads;

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

        void failCurrentStandardJobs(WorkflowExecutionFailureCause *cause);

        void processWorkCompletion(WorkerThread *worker_thread, std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> pre_file_copies,
                                   std::vector<WorkflowTask *> tasks,
                                   std::map<WorkflowFile *, StorageService *> file_locations,
                                   std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> post_file_copies);

        void processWorkFailure(WorkerThread *worker_thread, std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> pre_file_copies,
                                std::vector<WorkflowTask *> tasks,
                                std::map<WorkflowFile *, StorageService *> file_locations,
                                std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> post_file_copies, WorkflowExecutionFailureCause *);

        void processPilotJobCompletion(PilotJob *job);

        bool processNextMessage(double timeout);

        bool dispatchNextPendingTask();

        bool dispatchNextPendingJob();

        void failStandardJob(StandardJob *job, WorkflowExecutionFailureCause *cause);
    };
};


#endif //WRENCH_MULTICORECOMPUTESERVICE_H
