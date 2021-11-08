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

#include <wrench/action/Action.h>


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
    class FileRegistryService;
    class FileRegistryAddEntryAction;
    class FileRegistryDeleteEntryAction;
    class ParallelModel;
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

        void setServiceSpecificArgs(std::map<std::string, std::string> service_specific_args);

        const std::map<std::string, std::string> & getServiceSpecificArgs();

        bool hasSuccessfullyCompleted();
        bool hasFailed();

        /***********************/
        /** \endcond           */
        /***********************/

        /***********************/
        /** \cond INTERNAL    */
        /***********************/

    protected:

        friend class BareMetalComputeService;
        friend class JobManager;
        friend class Action;

        CompoundJob(std::string name, std::shared_ptr<JobManager> job_manager);

        bool isReady();

        std::shared_ptr<CompoundJob> shared_this; // Set by the Job Manager
        std::set<std::shared_ptr<Action>> actions;
        State state;
        unsigned long priority;

        void updateStateActionMap(std::shared_ptr<Action> action, Action::State old_state, Action::State new_state);

        void setAllActionsFailed(std::shared_ptr<FailureCause> cause);

    private:

        void assertJobNotSubmitted();

        bool pathExists(const std::shared_ptr<Action>& a, const std::shared_ptr<Action> &b);

        std::set<std::shared_ptr<CompoundJob>> parents;
        std::set<std::shared_ptr<CompoundJob>> children;

        std::map<std::string, std::string> service_specific_args;

        std::map<Action::State, std::set<std::shared_ptr<Action>>> state_task_map;

    };


    /***********************/
    /** \endcond           */
    /***********************/

};

#endif //WRENCH_COMPOUNDJOB_H
