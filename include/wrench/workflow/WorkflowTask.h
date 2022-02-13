/**
 * Copyright (c) 2017-2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_WORKFLOWTASK_H
#define WRENCH_WORKFLOWTASK_H

#include <map>
#include <stack>
#include <set>
#include <memory>


#include "wrench/job/Job.h"
#include "wrench/data_file/DataFile.h"
#include "wrench/workflow/parallel_model/ParallelModel.h"
#include "wrench/workflow/parallel_model/AmdahlParallelModel.h"
#include "wrench/workflow/parallel_model/ConstantEfficiencyParallelModel.h"
#include "wrench/workflow/parallel_model/CustomParallelModel.h"

#include <boost/graph/adjacency_list.hpp>

namespace wrench {

    /**
     * @brief A computational task in a Workflow
     */
    class WorkflowTask : public std::enable_shared_from_this<WorkflowTask> {

    public:
        const std::string& getID() const;

        double getFlops() const;

        unsigned long getMinNumCores() const;

        unsigned long getMaxNumCores() const;

        std::shared_ptr<ParallelModel> getParallelModel() const;

        void setParallelModel(std::shared_ptr<ParallelModel> model);

        double getMemoryRequirement() const;

        unsigned long getNumberOfChildren();

        std::vector<std::shared_ptr<WorkflowTask>> getChildren();

        unsigned long getNumberOfParents();

        std::vector<std::shared_ptr<WorkflowTask>> getParents();

        void addInputFile(std::shared_ptr<DataFile>file);

        void addOutputFile(std::shared_ptr<DataFile>file);

        unsigned int getFailureCount();

        std::shared_ptr<WorkflowTask> getSharedPtr() { return this->shared_from_this(); }


        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        /** @brief Task states */
        enum State {
            /** @brief Not ready (parents have not completed) */
            NOT_READY,
            /** @brief Ready (parents have completed) */
            READY,
            /** @brief Pending (has been submitted to a compute service) */
            PENDING,
            /** @brief Completed (successfully completed) */
            COMPLETED,
            /** @brief Some Unknown state (should not happen) */
            UNKNOWN
        };

        static std::string stateToString(WorkflowTask::State state);

        Job *getJob() const;

        std::shared_ptr<Workflow> getWorkflow() const;

        std::string getClusterID() const;

        void setClusterID(std::string);

        void setPriority(long);

        unsigned long getPriority() const;

        void setAverageCPU(double);

        double getAverageCPU() const;

        void setBytesRead(unsigned long);

        unsigned long getBytesRead() const;

        void setBytesWritten(unsigned long);

        unsigned long getBytesWritten() const;

        std::vector<std::shared_ptr<DataFile>> getInputFiles() const;

        std::vector<std::shared_ptr<DataFile>> getOutputFiles() const;

        unsigned long getTopLevel() const;

        double getStartDate() const;

        double getEndDate() const;

        double getFailureDate() const;

        double getTerminationDate() const;

        double getReadInputStartDate() const;

        double getReadInputEndDate() const;

        double getComputationStartDate() const;

        double getComputationEndDate() const;

        double getWriteOutputStartDate() const;

        double getWriteOutputEndDate() const;

        unsigned long getNumCoresAllocated() const;

        struct WorkflowTaskExecution;

        std::stack<WorkflowTaskExecution> getExecutionHistory() const;

        std::string getExecutionHost() const;

        std::string getPhysicalExecutionHost()const ;

        WorkflowTask::State getState() const;

        std::string getStateAsString() const;

        std::string getColor() const;

        void setColor(std::string);

        /***********************/
        /** \endcond           */
        /***********************/


        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        /** @brief Task state enum */
        enum InternalState {
            TASK_NOT_READY,
            TASK_READY,
            TASK_RUNNING,
            TASK_COMPLETED,
            TASK_FAILED
        };

        void updateReadiness();

        static std::string stateToString(WorkflowTask::InternalState state);

        unsigned long updateTopLevel();

        void setInternalState(WorkflowTask::InternalState);

        void setState(WorkflowTask::State);

//        void setUpcomingState(WorkflowTask::State);
//
//        WorkflowTask::State getUpcomingState() const;

        WorkflowTask::InternalState getInternalState() const;

        void setJob(Job *job);

        void setStartDate(double date);

        void updateStartDate(double date);

        void setEndDate(double date);

        void setReadInputStartDate(double date);

        void setReadInputEndDate(double date);

        void setComputationStartDate(double date);

        void setComputationEndDate(double date);

        void setWriteOutputStartDate(double date);

        void setWriteOutputEndDate(double date);

        void setFailureDate(double date);

        void setTerminationDate(double date);

        void incrementFailureCount();

        void setExecutionHost(std::string hostname);

        void setNumCoresAllocated(unsigned long num_cores);

        /**
         * @brief A data structure that keeps track of a task's execution event times
         */
        struct WorkflowTaskExecution {
            /** @brief Task's start time **/
            double task_start = -1.0;
            /** @brief Task's read input start time **/
            double read_input_start = -1.0;
            /** @brief Task's read input end time **/
            double read_input_end = -1.0;
            /** @brief Task's computation start time **/
            double computation_start = -1.0;
            /** @brief Task's computation end time **/
            double computation_end = -1.0;
            /** @brief Task's write output start time **/
            double write_output_start = -1.0;
            /** @brief Task's write output end time **/
            double write_output_end = -1.0;
            /** @brief Task's end time **/
            double task_end = -1.0;
            /** @brief Task's failed time **/
            double task_failed = -1.0;
            /** @brief Task's terminated time **/
            double task_terminated = -1.0;

            /** @brief Task's execution host (could be a virtual host)**/
            std::string execution_host = "";
            /** @brief Task's execution physical host **/
            std::string physical_execution_host = "";
            /** @brief Task's number of allocated cores **/
            unsigned long num_cores_allocated = 0;

            /**
             * @brief Constructor
             *
             * @param task_start: Task start time
             */
            WorkflowTaskExecution(double task_start) : task_start(task_start) {}


        };


        /***********************/
        /** \endcond           */
        /***********************/

    private:
        friend class Workflow;

        std::string id;                    // Task ID
        std::string cluster_id;            // ID for clustered task
        std::string color;                 // A RGB color formatted as "#rrggbb"
        double flops;                      // Number of flops
        double average_cpu = -1;           // Average CPU utilization
        unsigned long bytes_read = -1;     // Total bytes read in KB
        unsigned long bytes_written = -1;  // Total bytes written in KB
        unsigned long min_num_cores;
        unsigned long max_num_cores;
        std::shared_ptr<ParallelModel> parallel_model;
        double memory_requirement;
        unsigned long priority = 0;        // Task priority
        unsigned long toplevel;            // 0 if entry task
        unsigned int failure_count = 0;    // Number of times the tasks has failed
        std::string execution_host;        // Host on which the task executed ("" if not executed successfully - yet)
        State visible_state;               // To be exposed to developer level
        State upcoming_visible_state;      // A visible state that will become active once a WMS has process a previously sent workflow execution event
        InternalState internal_state;      // Not to be exposed to developer level

        std::shared_ptr<Workflow> workflow;                // Containing workflow

        std::map<std::string, std::shared_ptr<DataFile>> output_files;   // List of output files
        std::map<std::string, std::shared_ptr<DataFile>> input_files;    // List of input files

        // Private constructor (called by Workflow)
        WorkflowTask(std::string id,
                     double t,
                     unsigned long min_num_cores,
                     unsigned long max_num_cores,
                     double memory_requirement);

        // Containing job
        Job *job;

        std::stack<WorkflowTaskExecution> execution_history;

        friend class DagOfTasks;
    };
};

#endif //WRENCH_WORKFLOWTASK_H
