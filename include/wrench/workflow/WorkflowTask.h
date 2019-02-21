/**
 * Copyright (c) 2017-2019. The WRENCH Team.
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
#include <lemon/list_graph.h>
#include <set>

#include "wrench/workflow/job/WorkflowJob.h"
#include "wrench/workflow/WorkflowFile.h"

namespace wrench {

    /**
     * @brief A computational task in a Workflow
     */
    class WorkflowTask {

    public:
        std::string getID() const;

        double getFlops() const;

        unsigned long getMinNumCores() const;

        unsigned long getMaxNumCores() const;

        double getParallelEfficiency() const;

        double getMemoryRequirement() const;

        int getNumberOfChildren() const;

        int getNumberOfParents() const;

        void addInputFile(WorkflowFile *file);

        void addOutputFile(WorkflowFile *file);

        unsigned int getFailureCount();

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        /** @brief Task types */
        enum TaskType {
            COMPUTE,
            AUXILIARY,
            TRANSFER
        };

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

        WorkflowJob *getJob() const;

        Workflow *getWorkflow() const;

        std::string getClusterID() const;

        void setClusterID(std::string);

        void setTaskType(TaskType);

        TaskType getTaskType() const;

        void setPriority(long);

        long getPriority() const;

        void setAverageCPU(double);

        double getAverageCPU() const;

        void setBytesRead(unsigned long);

        unsigned long getBytesRead() const;

        void setBytesWritten(unsigned long);

        unsigned long getBytesWritten() const;

        std::set<WorkflowFile *> getInputFiles();

        std::set<WorkflowFile *> getOutputFiles();

        unsigned long getTopLevel();

        double getStartDate();

        double getEndDate();

        double getFailureDate();

        double getTerminationDate();

        double getReadInputStartDate();

        double getReadInputEndDate();

        double getComputationStartDate();

        double getComputationEndDate();

        double getWriteOutputStartDate();

        double getWriteOutputEndDate();

        unsigned long getNumCoresAllocated();

        struct WorkflowTaskExecution;

        std::stack<WorkflowTaskExecution> getExecutionHistory();

        std::string getExecutionHost();

        WorkflowTask::State getState() const;

        void addSrcDest(WorkflowFile *, const std::string &, const std::string &);

        std::map<WorkflowFile *, std::pair<std::string, std::string>> getFileTransfers() const;

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

        static std::string stateToString(WorkflowTask::InternalState state);

        unsigned long updateTopLevel();

        void setInternalState(WorkflowTask::InternalState);

        void setState(WorkflowTask::State);

        void setUpcomingState(WorkflowTask::State);

        WorkflowTask::State getUpcomingState() const;

        WorkflowTask::InternalState getInternalState() const;

        void setJob(WorkflowJob *job);

        void setStartDate(double date);

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

        struct WorkflowTaskExecution {
            double task_start = -1.0;
            double read_input_start = -1.0;
            double read_input_end = -1.0;
            double computation_start = -1.0;
            double computation_end = -1.0;
            double write_output_start = -1.0;
            double write_output_end = -1.0;
            double task_end = -1.0;
            double task_failed = -1.0;
            double task_terminated = -1.0;

            std::string execution_host = "";
            unsigned long num_cores_allocated = 0;

            WorkflowTaskExecution(double task_start) : task_start(task_start) {

            }
        };

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        friend class Workflow;

        std::string id;                    // Task ID
        std::string cluster_id;            // ID for clustered task
        TaskType task_type;                // Task type
        double flops;                      // Number of flops
        double average_cpu;                // Average CPU utilization
        unsigned long bytes_read = -1;     // Total bytes read in KB
        unsigned long bytes_written = -1;  // Total bytes written in KB
        unsigned long min_num_cores;
        unsigned long max_num_cores;
        double parallel_efficiency;
        double memory_requirement;
        long priority = 0;                 // Task priority
        unsigned long toplevel;            // 0 if entry task
        unsigned int failure_count = 0;    // Number of times the tasks has failed
        std::string execution_host;        // Host on which the task executed ("" if not executed successfully - yet)
        State visible_state;               // To be exposed to developer level
        State upcoming_visible_state;      // A visible state that will become active once a WMS has process a previously sent workflow execution event
        InternalState internal_state;      // Not to be exposed to developer level

        Workflow *workflow;                                   // Containing workflow
        lemon::ListDigraph *DAG;                              // Containing workflow
        lemon::ListDigraph::Node DAG_node;                    // pointer to the underlying DAG node
        std::map<std::string, WorkflowFile *> output_files;   // List of output files
        std::map<std::string, WorkflowFile *> input_files;    // List of input files
        std::map<WorkflowFile *, std::pair<std::string, std::string>> fileTransfers;  // Map of transfer files and hosts

        // Private constructor (called by Workflow)
        WorkflowTask(std::string id,
                     double t,
                     unsigned long min_num_cores,
                     unsigned long max_num_cores,
                     double parallel_efficiency,
                     double memory_requirement,
                     TaskType type);

        // Containing job
        WorkflowJob *job;

        // Private helper function
        void addFileToMap(std::map<std::string, WorkflowFile *> &map_to_insert,
                          std::map<std::string, WorkflowFile *> &map_to_check,
                          WorkflowFile *f);

        std::stack<WorkflowTaskExecution> execution_history;
    };
};

#endif //WRENCH_WORKFLOWTASK_H
