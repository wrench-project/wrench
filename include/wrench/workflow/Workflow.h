/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_WORKFLOW_H
#define WRENCH_WORKFLOW_H

#include <lemon/list_graph.h>
#include <map>
#include <set>

#include "wrench/workflow/execution_events/WorkflowExecutionEvent.h"
#include "WorkflowFile.h"
#include "WorkflowTask.h"

class WorkflowTask;

namespace wrench {

    /**
     * @brief A workflow (to be executed by a WMS)
     */
    class Workflow {

    public:
        Workflow();

        WorkflowTask *addTask(std::string, double flops, int min_num_cores = 1,
                              int max_num_cores = 1,
                              double parallel_efficiency = 1.0,
                              double memory_requirement = 0.0);

        void removeTask(WorkflowTask *task);

        WorkflowTask *getWorkflowTaskByID(std::string);

        WorkflowFile *addFile(std::string, double);

        WorkflowFile *getFileById(std::string id);

        WorkflowFile *getWorkflowFileByID(std::string);

        static double getSumFlops(std::vector<WorkflowTask *> tasks);

        void addControlDependency(WorkflowTask *, WorkflowTask *);

        void loadFromDAX(const std::string &filename, const std::string &reference_flop_rate);

        void loadFromJSON(const std::string &filename, const std::string &reference_flop_rate);

        unsigned long getNumberOfTasks();

        unsigned long getNumLevels();

        double getCompletionDate();

        void exportToEPS(std::string);

        std::map<std::string, WorkflowFile *> getInputFiles();

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        bool isDone();

        std::vector<WorkflowTask *> getTasksInTopLevelRange(unsigned long min, unsigned long max);

        std::vector<WorkflowTask *> getReadyTasks();

        std::map<std::string, std::vector<WorkflowTask *>> getReadyClusters();

        std::vector<WorkflowTask *> getTasks();

        std::vector<WorkflowFile *> getFiles();

        std::vector<WorkflowTask *> getTaskParents(const WorkflowTask *task);

        std::vector<WorkflowTask *> getTaskChildren(const WorkflowTask *task);

        std::unique_ptr<WorkflowExecutionEvent> waitForNextExecutionEvent();

        /***********************/
        /** \endcond           */
        /***********************/


        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        std::string getCallbackMailbox();

//        void updateTaskState(WorkflowTask *task, WorkflowTask::State state);

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        friend class WorkflowTask;

        void setNumLevels(unsigned long);

        std::unique_ptr<lemon::ListDigraph> DAG;  // Lemon DiGraph
        std::unique_ptr<lemon::ListDigraph::NodeMap<WorkflowTask *>> DAG_node_map;  // Lemon map

        std::map<std::string, std::unique_ptr<WorkflowTask>> tasks;
        std::map<std::string, std::unique_ptr<WorkflowFile>> files;

        unsigned long num_levels;

        bool pathExists(WorkflowTask *, WorkflowTask *);

        std::string callback_mailbox;

        ComputeService *parent_compute_service; // The compute service to which the job was submitted, if any
    };
};

#endif //WRENCH_WORKFLOW_H
