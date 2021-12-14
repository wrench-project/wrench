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
#include <memory>

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

    class DataFile;

    /**
     * @brief A compound job
     */
class CompoundJob : public Job, public std::enable_shared_from_this<CompoundJob> {

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
             * Actions may have failed, the job may have been terminated/killed, or parent
             * jobs may have been discontinued.
             */
            DISCONTINUED
        };

        /**
         * @brief Get the shared pointer for this object
         * @return a shared pointer to the object
         */
        std::shared_ptr<CompoundJob> getSharedPtr() { return this->shared_from_this(); }

        std::set<std::shared_ptr<Action>> getActions();
        CompoundJob::State getState();
        std::string getStateAsString();
        void setPriority(double priority) override;

        std::shared_ptr<SleepAction> addSleepAction(std::string name, double sleep_time);

        std::shared_ptr<FileReadAction> addFileReadAction(std::string name,
                                                          std::shared_ptr<DataFile>file,
                                                          std::shared_ptr<FileLocation> file_location);

        std::shared_ptr<FileReadAction> addFileReadAction(std::string name,
                                                          std::shared_ptr<DataFile>file,
                                                          std::vector<std::shared_ptr<FileLocation>> file_locations);

        std::shared_ptr<FileWriteAction> addFileWriteAction(std::string name,
                                                            std::shared_ptr<DataFile>file,
                                                            std::shared_ptr<FileLocation> file_location);

        std::shared_ptr<FileCopyAction> addFileCopyAction(std::string name,
                                                            std::shared_ptr<DataFile>file,
                                                            std::shared_ptr<FileLocation> src_file_location,
                                                            std::shared_ptr<FileLocation> dst_file_location);

        std::shared_ptr<FileDeleteAction> addFileDeleteAction(std::string name,
                                                          std::shared_ptr<DataFile>file,
                                                          std::shared_ptr<FileLocation> file_location);

        std::shared_ptr<FileRegistryAddEntryAction> addFileRegistryAddEntryAction(std::shared_ptr<FileRegistryService> file_registry, std::shared_ptr<DataFile>file,
                                                                                std::shared_ptr<FileLocation> file_location);

        std::shared_ptr<FileRegistryDeleteEntryAction> addFileRegistryDeleteEntryAction(std::shared_ptr<FileRegistryService> file_registry, std::shared_ptr<DataFile>file,
                                                                                   std::shared_ptr<FileLocation> file_location);

        std::shared_ptr<ComputeAction> addComputeAction(std::string name,
                                                        double flops,
                                                        double ram,
                                                        unsigned long min_num_cores,
                                                        unsigned long max_num_cores,
                                                        std::shared_ptr<ParallelModel> parallel_model);

        std::shared_ptr<CustomAction> addCustomAction(std::string name,
                                                      const std::function<void (std::shared_ptr<ActionExecutor> action_executor)> &lambda_execute,
                                                      const std::function<void (std::shared_ptr<ActionExecutor> action_executor)> &lambda_terminate);

        void removeAction(std::shared_ptr<Action> &action);

        void addActionDependency(const std::shared_ptr<Action>& parent, const std::shared_ptr<Action>& child);

        void addParentJob(const std::shared_ptr<CompoundJob>& parent);
        void addChildJob(const std::shared_ptr<CompoundJob>& child);

        std::set<std::shared_ptr<CompoundJob>> getParentJobs();
        std::set<std::shared_ptr<CompoundJob>> getChildrenJobs();

//        void setServiceSpecificArgs(std::map<std::string, std::string> service_specific_args);
//        const std::map<std::string, std::string> & getServiceSpecificArgs();

        bool hasSuccessfullyCompleted();
        bool hasFailed();

        void printActionDependencies();

        void printTaskMap();

        unsigned long getMinimumRequiredNumCores();

        double getMinimumRequiredMemory();

        bool usesScratch();

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

//        std::shared_ptr<CompoundJob> shared_this; // Set by the Job Manager
        std::set<std::shared_ptr<Action>> actions;
        std::map<std::string, std::shared_ptr<Action>> name_map;

        State state;
        unsigned long priority;

        void updateStateActionMap(const std::shared_ptr<Action>& action, Action::State old_state, Action::State new_state);

        void setAllActionsFailed(const std::shared_ptr<FailureCause>& cause);

        bool hasAction(const std::string &name);

        std::set<std::shared_ptr<CompoundJob>> &getChildren();
        std::set<std::shared_ptr<CompoundJob>> &getParents();

    private:

        double pre_job_overhead;
        double post_job_overhead;

        void assertJobNotSubmitted();
        void assertActionNameDoesNotAlreadyExist(const std::string &name);

        void addAction(const std::shared_ptr<Action>& action);

        bool pathExists(const std::shared_ptr<Action>& a, const std::shared_ptr<Action> &b);
        bool pathExists(const std::shared_ptr<CompoundJob>& a, const std::shared_ptr<CompoundJob> &b);

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
