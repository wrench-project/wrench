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
#include <wrench/services/storage/StorageService.h>

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
    class MPIAction;
    class ActionExecutor;
    class DataFile;
    /**
     * @brief A compound job class
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
        void setPriority(double p) override;

        ~CompoundJob() = default;

        std::shared_ptr<SleepAction> addSleepAction(const std::string &name, double sleep_time);

        std::shared_ptr<FileReadAction> addFileReadAction(const std::string &name,
                                                          const std::shared_ptr<DataFile> &file,
                                                          const std::shared_ptr<StorageService> &storageService);

        std::shared_ptr<FileReadAction> addFileReadAction(const std::string &name,
                                                          const std::shared_ptr<DataFile> &file,
                                                          const std::shared_ptr<StorageService> &storageService,
                                                          double num_bytes_to_read);

        std::shared_ptr<FileReadAction> addFileReadAction(const std::string &name,
                                                          const std::shared_ptr<FileLocation> &file_location);

        std::shared_ptr<FileReadAction> addFileReadAction(const std::string &name,
                                                          const std::vector<std::shared_ptr<FileLocation>> &file_locations);

        std::shared_ptr<FileReadAction> addFileReadAction(const std::string &name,
                                                          const std::shared_ptr<FileLocation> &file_location,
                                                          double num_bytes_to_read);

        std::shared_ptr<FileReadAction> addFileReadAction(const std::string &name,
                                                          const std::vector<std::shared_ptr<FileLocation>> &file_locations,
                                                          double num_bytes_to_read);

        std::shared_ptr<FileWriteAction> addFileWriteAction(const std::string &name,
                                                            const std::shared_ptr<DataFile> &file,
                                                            const std::shared_ptr<StorageService> &storageService);

        std::shared_ptr<FileCopyAction> addFileCopyAction(const std::string &name,
                                                          const std::shared_ptr<DataFile> &file,
                                                          const std::shared_ptr<StorageService> &src_storageService,
                                                          const std::shared_ptr<StorageService> &dest_storageService);

        std::shared_ptr<FileDeleteAction> addFileDeleteAction(const std::string &name,
                                                              const std::shared_ptr<DataFile> &file,
                                                              const std::shared_ptr<StorageService> &storageService);

        std::shared_ptr<FileWriteAction> addFileWriteAction(const std::string &name,
                                                            const std::shared_ptr<FileLocation> &file_location);

        std::shared_ptr<FileCopyAction> addFileCopyAction(const std::string &name,
                                                          const std::shared_ptr<FileLocation> &src_file_location,
                                                          const std::shared_ptr<FileLocation> &dst_file_location);

        std::shared_ptr<FileDeleteAction> addFileDeleteAction(const std::string &name,
                                                              const std::shared_ptr<FileLocation> &file_location);

        std::shared_ptr<FileRegistryAddEntryAction> addFileRegistryAddEntryAction(const std::string &name,
                                                                                  const std::shared_ptr<FileRegistryService> &file_registry,
                                                                                  const std::shared_ptr<FileLocation> &file_location);

        std::shared_ptr<FileRegistryDeleteEntryAction> addFileRegistryDeleteEntryAction(const std::string &name,
                                                                                        const std::shared_ptr<FileRegistryService> &file_registry,
                                                                                        const std::shared_ptr<FileLocation> &file_location);

        std::shared_ptr<ComputeAction> addComputeAction(const std::string &name,
                                                        double flops,
                                                        double ram,
                                                        unsigned long min_num_cores,
                                                        unsigned long max_num_cores,
                                                        const std::shared_ptr<ParallelModel> &parallel_model);

        std::shared_ptr<CustomAction> addCustomAction(const std::string &name,
                                                      double ram,
                                                      unsigned long num_cores,
                                                      const std::function<void(std::shared_ptr<ActionExecutor> action_executor)> &lambda_execute,
                                                      const std::function<void(std::shared_ptr<ActionExecutor> action_executor)> &lambda_terminate);

        std::shared_ptr<CustomAction> addCustomAction(std::shared_ptr<CustomAction> custom_action);

        std::shared_ptr<MPIAction> addMPIAction(const std::string &name,
                                                const std::function<void(const std::shared_ptr<ActionExecutor> &action_executor)> &mpi_code,
                                                unsigned long num_processes,
                                                unsigned long num_cores_per_process);

        void removeAction(std::shared_ptr<Action> &action);

        void addActionDependency(const std::shared_ptr<Action> &parent, const std::shared_ptr<Action> &child);

        void addParentJob(const std::shared_ptr<CompoundJob> &parent);
        void addChildJob(const std::shared_ptr<CompoundJob> &child);

        std::set<std::shared_ptr<CompoundJob>> getParentJobs();
        std::set<std::shared_ptr<CompoundJob>> getChildrenJobs();

        //        void setServiceSpecificArgs(std::map<std::string, std::string> service_specific_args);
        //        const std::map<std::string, std::string> & getServiceSpecificArgs();

        bool hasSuccessfullyCompleted();
        bool hasFailed();

        unsigned long getMinimumRequiredNumCores();

        double getMinimumRequiredMemory();


        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        bool usesScratch();

        void printActionDependencies();

        void printTaskMap();


    protected:
        friend class BareMetalComputeService;
        friend class JobManager;
        friend class Action;

        CompoundJob(std::string name, std::shared_ptr<JobManager> job_manager);

        bool isReady();

        /**
         * @brief Actions in the job
         */
        std::set<std::shared_ptr<Action>> actions;
        /**
         * @brief Map of action names to actions
         */
        std::unordered_map<std::string, std::shared_ptr<Action>> name_map;

        /**
         * @brief Job state
         */
        State state;
        /**
         * @brief Job priority
         */
        double priority;

        void updateStateActionMap(const std::shared_ptr<Action> &action, Action::State old_state, Action::State new_state);

        void setAllActionsFailed(const std::shared_ptr<FailureCause> &cause);

        bool hasAction(const std::string &name);

        std::set<std::shared_ptr<CompoundJob>> &getChildren();
        //        std::set<std::shared_ptr<CompoundJob>> &getParents();

    private:
        void assertJobNotSubmitted();
        void assertActionNameDoesNotAlreadyExist(const std::string &name);

        void addAction(const std::shared_ptr<Action> &action);

        bool pathExists(const std::shared_ptr<Action> &a, const std::shared_ptr<Action> &b);
        static bool pathExists(const std::shared_ptr<CompoundJob> &a, const std::shared_ptr<CompoundJob> &b);

        std::set<std::shared_ptr<CompoundJob>> parents;
        std::set<std::shared_ptr<CompoundJob>> children;

        std::map<std::string, std::string> service_specific_args;

        std::unordered_map<Action::State, std::set<std::shared_ptr<Action>>> state_task_map;

        /***********************/
        /** \endcond           */
        /***********************/
    };

    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench


#endif//WRENCH_COMPOUNDJOB_H
