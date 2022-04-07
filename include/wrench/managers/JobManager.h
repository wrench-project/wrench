/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_PILOTJOBMANAGER_H
#define WRENCH_PILOTJOBMANAGER_H

#include <vector>
#include <set>

#include "wrench/services/Service.h"
#include "wrench/services/storage/storage_helpers/FileLocation.h"


namespace wrench {

    class FailureCause;

    class WMS;

    class ExecutionController;

    class Workflow;

    class WorkflowTask;

    class DataFile;

    class Job;

    class PilotJob;

    class CompoundJob;

    class StandardJob;

    class ComputeService;

    class StorageService;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/


    /**
 * @brief A helper daemon (co-located with and explicitly started by an execution controller), which is used to
 *        handle all job executions
 */
    class JobManager : public Service {

    public:
        void stop() override;

        void kill();

        std::shared_ptr<CompoundJob> createCompoundJob(std::string name);

        std::shared_ptr<StandardJob> createStandardJob(const std::vector<std::shared_ptr<WorkflowTask>> &tasks,
                                                       const std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> &file_locations,
                                                       std::vector<std::tuple<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>>> pre_file_copies,
                                                       std::vector<std::tuple<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>>> post_file_copies,
                                                       std::vector<std::tuple<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>>> cleanup_file_deletions);

        std::shared_ptr<StandardJob> createStandardJob(const std::vector<std::shared_ptr<WorkflowTask>> &tasks,
                                                       std::map<std::shared_ptr<DataFile>, std::vector<std::shared_ptr<FileLocation>>> file_locations,
                                                       std::vector<std::tuple<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>>> pre_file_copies,
                                                       std::vector<std::tuple<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>>> post_file_copies,
                                                       std::vector<std::tuple<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>>> cleanup_file_deletions);


        std::shared_ptr<StandardJob> createStandardJob(const std::vector<std::shared_ptr<WorkflowTask>> &tasks,
                                                       const std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> &file_locations);

        std::shared_ptr<StandardJob> createStandardJob(const std::vector<std::shared_ptr<WorkflowTask>> &tasks,
                                                       std::map<std::shared_ptr<DataFile>, std::vector<std::shared_ptr<FileLocation>>> file_locations);

        std::shared_ptr<StandardJob> createStandardJob(const std::shared_ptr<WorkflowTask> &task,
                                                       const std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> &file_locations);

        std::shared_ptr<StandardJob> createStandardJob(const std::shared_ptr<WorkflowTask> &task,
                                                       std::map<std::shared_ptr<DataFile>, std::vector<std::shared_ptr<FileLocation>>> file_locations);

        std::shared_ptr<StandardJob> createStandardJob(const std::vector<std::shared_ptr<WorkflowTask>> &tasks);

        std::shared_ptr<StandardJob> createStandardJob(const std::shared_ptr<WorkflowTask> &task);

        std::shared_ptr<PilotJob> createPilotJob();

        void submitJob(const std::shared_ptr<StandardJob> &job, const std::shared_ptr<ComputeService> &compute_service,
                       std::map<std::string, std::string> service_specific_args = {});

        void submitJob(const std::shared_ptr<CompoundJob> &job, const std::shared_ptr<ComputeService> &compute_service,
                       std::map<std::string, std::string> service_specific_args = {});

        void submitJob(const std::shared_ptr<PilotJob> &job, const std::shared_ptr<ComputeService> &compute_service,
                       std::map<std::string, std::string> service_specific_args = {});

        void terminateJob(const std::shared_ptr<StandardJob> &job);

        void terminateJob(const std::shared_ptr<CompoundJob> &job);

        void terminateJob(const std::shared_ptr<PilotJob> &job);

        simgrid::s4u::Mailbox *getCreatorMailbox();

        unsigned long getNumRunningPilotJobs() const;

        /***********************/
        /** \cond INTERNAL    */
        /***********************/

        ~JobManager() override;

    protected:
        friend class ExecutionController;
        friend class WMS;

        explicit JobManager(std::string hostname, simgrid::s4u::Mailbox *creator_mailbox);

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        int main() override;

        void dispatchJobs();

        void dispatchJob(const std::shared_ptr<CompoundJob> &job);

        bool processNextMessage();

        void
        processStandardJobCompletion(const std::shared_ptr<StandardJob> &job, std::shared_ptr<ComputeService> compute_service);

        void
        processStandardJobFailure(const std::shared_ptr<StandardJob>& job, std::shared_ptr<ComputeService> compute_service);

        void
        processCompoundJobCompletion(const std::shared_ptr<CompoundJob> &job, std::shared_ptr<ComputeService> compute_service);

        void
        processCompoundJobFailure(const std::shared_ptr<CompoundJob> &job, std::shared_ptr<ComputeService> compute_service);

        void processPilotJobStart(const std::shared_ptr<PilotJob> &job, std::shared_ptr<ComputeService> compute_service);

        void processPilotJobExpiration(const std::shared_ptr<PilotJob> &job, std::shared_ptr<ComputeService> compute_service);

        void processPilotJobFailure(const std::shared_ptr<PilotJob> &job, std::shared_ptr<ComputeService> compute_service, std::shared_ptr<FailureCause> cause);

        // Mailbox of the creator of this job manager
        simgrid::s4u::Mailbox *creator_mailbox;

        std::vector<std::shared_ptr<CompoundJob>> jobs_to_dispatch;
        std::set<std::shared_ptr<CompoundJob>> jobs_dispatched;

        unsigned long num_running_pilot_jobs = 0;

        //        std::map<std::shared_ptr<CompoundJob>, std::map<std::string, std::string>> cjob_args;

        std::map<std::shared_ptr<CompoundJob>, std::shared_ptr<StandardJob>> cjob_to_sjob_map;
        std::map<std::shared_ptr<CompoundJob>, std::shared_ptr<PilotJob>> cjob_to_pjob_map;
    };

    /***********************/
    /** \endcond            */
    /***********************/

};// namespace wrench

#endif//WRENCH_PILOTJOBMANAGER_H
