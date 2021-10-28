/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_COMPOUNDJOB_H
#define WRENCH_COMPOUNDJOB_H

#include <map>
#include <set>
#include <vector>


#include "Job.h"

namespace wrench {

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    class Action;
    class SleepAction;
    class ComputeAction;
    class FileReadAction;
    class FileWriteAction;
    class FileCopyAction;
    class FileDeleteAction;
    class FileRegistryAddEntryAction;
    class FileRegistryDeleteEntryAction;
    class CustomAction;
    class ActionExecutor;

    /**
     * @brief A compound job
     */
    class CompoundJob : public Job {

    public:

        /** @brief Compound job states */
        enum State {
            /** @brief Job hasn't been submitted yet **/
            NOT_SUBMITTED,
            /** @brief Job has been submitted to a JobManager **/
            SUBMITTED,
            /** @brief Job has finished executing and all actions were successfully completed **/
            COMPLETED,
            /** @brief Job has finished executing but not all actions were successfully completed.
             * Actions may have failed, they job may have been terminated/killed, or parent
             * jobs by have been discontinued.
             */
            DISCONTINUED
        };

        std::set<std::shared_ptr<Action>> getActions();
        CompoundJob::State getState();
        unsigned long getPriority() override;
        void setPriority(unsigned long priority);

        std::shared_ptr<SleepAction> addSleepAction(std::string name, double sleep_time);

        std::shared_ptr<FileReadAction> addFileReadAction(std::string name,
                                                          WorkflowFile *file,
                                                          std::shared_ptr<FileLocation> file_location);

        std::shared_ptr<FileReadAction> addFileReadAction(std::string name,
                                                          WorkflowFile *file,
                                                          std::vector<std::shared_ptr<FileLocation>> file_locations);

        std::shared_ptr<FileWriteAction> addFileWriteAction(std::string name,
                                                            WorkflowFile *file,
                                                            std::shared_ptr<FileLocation> file_location);

        std::shared_ptr<FileCopyAction> addFileCopyAction(std::string name,
                                                            WorkflowFile *file,
                                                            std::shared_ptr<FileLocation> src_file_location,
                                                            std::shared_ptr<FileLocation> dst_file_location);

        std::shared_ptr<FileDeleteAction> addFileDeleteAction(std::string name,
                                                          WorkflowFile *file,
                                                          std::shared_ptr<FileLocation> file_location);

        std::shared_ptr<FileRegistryAddEntryAction> addFileRegistryAddEntryAction(std::shared_ptr<FileRegistryService> file_registry, WorkflowFile *file,
                                                                                std::shared_ptr<FileLocation> file_location);

        std::shared_ptr<FileRegistryDeleteEntryAction> addFileRegistryDeleteEntryAction(std::shared_ptr<FileRegistryService> file_registry, WorkflowFile *file,
                                                                                   std::shared_ptr<FileLocation> file_location);

        std::shared_ptr<ComputeAction> addComputeAction(std::string name,
                                                        double flops,
                                                        double ram,
                                                        int min_num_cores,
                                                        int max_num_cores,
                                                        std::shared_ptr<ParallelModel> parallel_model);

        std::shared_ptr<CustomAction> addCustomAction(std::string name,
                                                      const std::function<void (std::shared_ptr<ActionExecutor> action_executor)> &lambda_execute,
                                                      const std::function<void (std::shared_ptr<ActionExecutor> action_executor)> &lambda_terminate);

        void addActionDependency(const std::shared_ptr<Action>& parent, const std::shared_ptr<Action>& child);

        void addParentJob(std::shared_ptr<CompoundJob> parent);
        void addChildJob(std::shared_ptr<CompoundJob> child);

        std::set<std::shared_ptr<CompoundJob>> getParentJobs();
        std::set<std::shared_ptr<CompoundJob>> getChildrenJobs();

        /***********************/
        /** \endcond           */
        /***********************/

        /***********************/
        /** \cond INTERNAL    */
        /***********************/

    protected:

        friend class JobManager;

        CompoundJob(std::string name, std::shared_ptr<JobManager> job_manager);

        bool isReady();

        std::shared_ptr<CompoundJob> shared_this; // Set by the Job Manager
        std::set<std::shared_ptr<Action>> actions;
        State state;
        unsigned long priority;

    private:
        std::set<std::shared_ptr<CompoundJob>> parents;
        std::set<std::shared_ptr<CompoundJob>> children;

    };


    /***********************/
    /** \endcond           */
    /***********************/

};

#endif //WRENCH_MULTITASKJOB_H
