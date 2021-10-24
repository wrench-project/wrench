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

    class Workflow;

    class WorkflowTask;

    class WorkflowFile;

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
 * @brief A helper daemon (co-located with and explicitly started by a WMS), which is used to
 *        handle all job executions
 */
    class JobManager : public Service {

    public:


        void stop() override;

        void kill();

        std::shared_ptr<CompoundJob> createCompoundJob(std::string name);

        std::shared_ptr<StandardJob> createStandardJob(std::vector<WorkflowTask *> tasks,
                                                       std::map<WorkflowFile *, std::shared_ptr<FileLocation> > file_locations,
                                                       std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>  >> pre_file_copies,
                                                       std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>  >> post_file_copies,
                                                       std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>  >> cleanup_file_deletions);

        std::shared_ptr<StandardJob> createStandardJob(std::vector<WorkflowTask *> tasks,
                                                       std::map<WorkflowFile *, std::vector<std::shared_ptr<FileLocation>>> file_locations,
                                                       std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>  >> pre_file_copies,
                                                       std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>  >> post_file_copies,
                                                       std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>  >> cleanup_file_deletions);


        std::shared_ptr<StandardJob> createStandardJob(std::vector<WorkflowTask *> tasks,
                                                       std::map<WorkflowFile *, std::shared_ptr<FileLocation>> file_locations);

        std::shared_ptr<StandardJob> createStandardJob(std::vector<WorkflowTask *> tasks,
                                                       std::map<WorkflowFile *, std::vector<std::shared_ptr<FileLocation>>> file_locations);

        std::shared_ptr<StandardJob> createStandardJob(WorkflowTask *task,
                                                       std::map<WorkflowFile *, std::shared_ptr<FileLocation>> file_locations);

        std::shared_ptr<StandardJob> createStandardJob(WorkflowTask *task,
                                                       std::map<WorkflowFile *, std::vector<std::shared_ptr<FileLocation>>> file_locations);

        std::shared_ptr<StandardJob> createStandardJob(std::vector<WorkflowTask *> tasks);

        std::shared_ptr<StandardJob> createStandardJob(WorkflowTask *task);

        std::shared_ptr<PilotJob> createPilotJob();

        void submitJob(std::shared_ptr<Job> job, std::shared_ptr<ComputeService> compute_service,
                       std::map<std::string, std::string> service_specific_args = {});

        void terminateJob(std::shared_ptr<Job> job);

//        void forgetJob(Job *job);

        std::set<std::shared_ptr<PilotJob>> getPendingPilotJobs();

        std::set<std::shared_ptr<PilotJob>> getRunningPilotJobs();

        /***********************/
        /** \cond INTERNAL    */
        /***********************/

        ~JobManager() override;

    protected:

        friend class WMS;

        explicit JobManager(std::shared_ptr<WMS> wms);

        /***********************/
        /** \endcond           */
        /***********************/

    private:

        int main() override;

        bool processNextMessage();

        void
        processStandardJobCompletion(std::shared_ptr<StandardJob> job, std::shared_ptr<ComputeService> compute_service);

        void
        processStandardJobFailure(std::shared_ptr<StandardJob> job, std::shared_ptr<ComputeService> compute_service,
                                  std::shared_ptr<FailureCause> cause);

        void processPilotJobStart(std::shared_ptr<PilotJob> job, std::shared_ptr<ComputeService> compute_service);

        void processPilotJobExpiration(std::shared_ptr<PilotJob> job, std::shared_ptr<ComputeService> compute_service);


        // Relevant WMS
        std::shared_ptr<WMS> wms;

        // Job lists
        std::set<std::shared_ptr<StandardJob>> new_standard_jobs;
        std::set<std::shared_ptr<StandardJob>> pending_standard_jobs;
        std::set<std::shared_ptr<StandardJob>> running_standard_jobs;
        std::set<std::shared_ptr<StandardJob>> completed_standard_jobs;
        std::set<std::shared_ptr<StandardJob>> failed_standard_jobs;

        std::set<std::shared_ptr<PilotJob>> new_pilot_jobs;
        std::set<std::shared_ptr<PilotJob>> pending_pilot_jobs;
        std::set<std::shared_ptr<PilotJob>> running_pilot_jobs;
        std::set<std::shared_ptr<PilotJob>> completed_pilot_jobs;

    };

    /***********************/
    /** \endcond            */
    /***********************/

};

#endif //WRENCH_PILOTJOBMANAGER_H
