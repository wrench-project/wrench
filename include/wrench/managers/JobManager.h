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

namespace wrench {

    class WMS;
    class Workflow;
    class WorkflowTask;
    class WorkflowFile;
    class WorkflowJob;
    class PilotJob;
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


        StandardJob *createStandardJob(std::vector<WorkflowTask *> tasks,
                                       std::map<WorkflowFile *, std::shared_ptr<FileLocation>  > file_locations,
                                       std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>  , std::shared_ptr<FileLocation>  >> pre_file_copies,
                                       std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>  , std::shared_ptr<FileLocation>  >> post_file_copies,
                                       std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>  >> cleanup_file_deletions);


        StandardJob *createStandardJob(std::vector<WorkflowTask *> tasks,
                                       std::map<WorkflowFile *,
                                               std::shared_ptr<FileLocation>  > file_locations);

        StandardJob *createStandardJob(WorkflowTask *task,
                                       std::map<WorkflowFile *,
                                               std::shared_ptr<FileLocation>  > file_locations);

        PilotJob *createPilotJob();

        void submitJob(WorkflowJob *job, std::shared_ptr<ComputeService> compute_service, std::map<std::string, std::string> service_specific_args = {});

        void terminateJob(WorkflowJob *);

        void forgetJob(WorkflowJob *job);

        std::set<PilotJob *> getPendingPilotJobs();

        std::set<PilotJob *> getRunningPilotJobs();
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
        void processStandardJobCompletion(StandardJob *job, std::shared_ptr<ComputeService> compute_service);
        void processStandardJobFailure(StandardJob *job, std::shared_ptr<ComputeService> compute_service, std::shared_ptr<FailureCause> cause);
        void processPilotJobStart(PilotJob *job, std::shared_ptr<ComputeService> compute_service);
        void processPilotJobExpiration(PilotJob *job, std::shared_ptr<ComputeService> compute_service);



        // Relevant WMS
        std::shared_ptr<WMS> wms;

        // Job map
        std::map<WorkflowJob*, std::unique_ptr<WorkflowJob>> jobs;

        // Job lists
        std::set<StandardJob *> pending_standard_jobs;
        std::set<StandardJob *> running_standard_jobs;
        std::set<StandardJob *> completed_standard_jobs;
        std::set<StandardJob *> failed_standard_jobs;

        std::set<PilotJob *> pending_pilot_jobs;
        std::set<PilotJob *> running_pilot_jobs;
        std::set<PilotJob *> completed_pilot_jobs;

    };

    /***********************/
    /** \endcond            */
    /***********************/

};

#endif //WRENCH_PILOTJOBMANAGER_H
