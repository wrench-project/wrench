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

    /**
     * @brief A compound job
     */
    class CompoundJob : public Job {

    public:

        /** @brief Compound job states */
        enum State {
            /** @brief Not submitted yet */
                    NOT_SUBMITTED,
            /** @brief Submitted but not running yet */
                    PENDING,
            /** @brief Running */
                    RUNNING,
            /** @brief Completed successfully */
                    COMPLETED,
            /** @brief Failed */
                    FAILED,
            /** @brief Terminated by submitter */
                    TERMINATED
        };

        std::set<std::shared_ptr<Action>> getActions();
        CompoundJob::State getState();
        unsigned long getPriority() override;
        void setPriority(unsigned long priority);

        std::shared_ptr<SleepAction> addSleepAction(std::string name, double sleep_time);

        std::shared_ptr<FileReadAction> addFileReadAction(std::string name,
                                                       std::shared_ptr<WorkflowFile> file,
                                                       std::shared_ptr<FileLocation> file_location);

        std::shared_ptr<FileReadAction> addFileReadAction(std::string name,
                                                          std::shared_ptr<WorkflowFile> file,
                                                          std::vector<std::shared_ptr<FileLocation>> file_locations);

        std::shared_ptr<ComputeAction> addComputeAction(std::string name,
                                                      double flops,
                                                      double ram,
                                                      int min_num_cores,
                                                      int max_num_cores,
                                                      std::shared_ptr<ParallelModel> parallel_model);

        void addDependency(std::shared_ptr<Action> parent, std::shared_ptr<Action> child);
        void removeDependency(std::shared_ptr<Action> parent, std::shared_ptr<Action> child);

        /***********************/
        /** \endcond           */
        /***********************/

        /***********************/
        /** \cond INTERNAL    */
        /***********************/

    protected:

        friend class JobManager;

        std::shared_ptr<CompoundJob> shared_this; // Set by the Job Manager

        CompoundJob(std::string name, std::shared_ptr<JobManager> job_manager);

        std::set<std::shared_ptr<Action>> actions;

        State state;
        unsigned long priority;

    };


    /***********************/
    /** \endcond           */
    /***********************/

};

#endif //WRENCH_MULTITASKJOB_H
