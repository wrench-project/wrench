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

#include "workflow_job/WorkflowJob.h"
#include "workflow/WorkflowFile.h"

namespace wrench {

    /**
     * @brief A workflow computational task
     */
    class WorkflowTask {

    public:
        std::string getId() const;

        double getFlops() const;

        int getNumProcs() const;

        int getNumberOfChildren() const;

        int getNumberOfParents() const;

        void addInputFile(WorkflowFile *file);

        void addOutputFile(WorkflowFile *file);

        unsigned int getFailureCount();

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        /** @brief Task state enum */
        enum State {
            NOT_READY,
            READY,
            PENDING,
            RUNNING,
            COMPLETED,
            FAILED
        };

        static std::string stateToString(WorkflowTask::State state);

        WorkflowTask::State getState() const;

        WorkflowJob *getJob() const;

        Workflow *getWorkflow() const;

        std::string getClusterId() const;

        void setClusterId(std::string);

        std::set<WorkflowFile *> getInputFiles();
        std::set<WorkflowFile *> getOutputFiles();

        double getStartDate();

        double getEndDate();


        /***********************/
        /** \endcond           */
        /***********************/



        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        void setState(WorkflowTask::State);

        void setReady();

        void setFailed();

        void setRunning();

        void setCompleted();

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
        int number_of_processors;          // currently vague: cores? nodes?
        double start_date = -1.0;          // Date at which task began execution (getter?)
        double end_date = -1.0;            // Date at which task finished execution (getter?)
        unsigned int failure_count = 0;    // Number of times the tasks has failed

        State state;
        Workflow *workflow;                                    // Containing workflow
        lemon::ListDigraph *DAG;                              // Containing workflow
        lemon::ListDigraph::Node DAG_node;                    // pointer to the underlying DAG node
        std::map<std::string, WorkflowFile *> output_files;    // List of output files
        std::map<std::string, WorkflowFile *> input_files;    // List of input files

        // Private constructor (called by Workflow)
        WorkflowTask(const std::string id, const double t, const int n);

        // Containing job
        WorkflowJob *job;

        // Private helper function
        void addFileToMap(std::map<std::string, WorkflowFile *> &map_to_insert,
                          std::map<std::string, WorkflowFile *> &map_to_check,
                          WorkflowFile *f);
    };
};

#endif //WRENCH_WORKFLOWTASK_H
