/**
 * Copyright (c) 2017. The WRENCH Team.
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
        std::string getId() const;

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


        /** @brief Task state enum */
        enum VisibleState {
            NOT_READY,
            READY,
            PENDING,
            COMPLETED
        };

        static std::string stateToString(WorkflowTask::VisibleState state);


        WorkflowJob *getJob() const;

        Workflow *getWorkflow() const;

        std::string getClusterId() const;

        void setClusterId(std::string);

        std::set<WorkflowFile *> getInputFiles();
        std::set<WorkflowFile *> getOutputFiles();
        unsigned long getTopLevel();

        double getStartDate();

        double getEndDate();

        WorkflowTask::VisibleState getState() const;


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

        void updateTopLevel();

        void setInternalState(WorkflowTask::InternalState);
        void setVisibleState(WorkflowTask::VisibleState);
        WorkflowTask::InternalState getInternalState() const;

        void setJob(WorkflowJob *job);

        void setStartDate(double date);

        void setEndDate(double date);

        void incrementFailureCount();

        /***********************/
        /** \endcond           */
        /***********************/


    private:

        friend class Workflow;

        std::string id;                    // Task ID
        std::string cluster_id;            // ID for clustered task
        double flops;                      // Number of flops
        unsigned long min_num_cores;
        unsigned long max_num_cores;
        double parallel_efficiency;
        double memory_requirement;

        unsigned long toplevel;           // 0 if entry task

        double start_date = -1.0;          // Date at which task began execution (getter?)
        double end_date = -1.0;            // Date at which task finished execution (getter?)
        unsigned int failure_count = 0;    // Number of times the tasks has failed

        VisibleState visible_state;              // To be exposed to developer level
        InternalState internal_state;              // Not to be exposed to developer level
        Workflow *workflow;                                    // Containing workflow
        lemon::ListDigraph *DAG;                              // Containing workflow
        lemon::ListDigraph::Node DAG_node;                    // pointer to the underlying DAG node
        std::map<std::string, WorkflowFile *> output_files;    // List of output files
        std::map<std::string, WorkflowFile *> input_files;    // List of input files

        // Private constructor (called by Workflow)
        WorkflowTask(const std::string id,
                     const double t,
                     const unsigned long min_num_cores,
                     const unsigned long max_num_cores,
                     const double parallel_efficiency,
                     const double memory_requirement);

        // Containing job
        WorkflowJob *job;

        // Private helper function
        void addFileToMap(std::map<std::string, WorkflowFile *> &map_to_insert,
                          std::map<std::string, WorkflowFile *> &map_to_check,
                          WorkflowFile *f);
    };
};

#endif //WRENCH_WORKFLOWTASK_H
