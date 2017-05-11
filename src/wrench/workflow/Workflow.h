/**
 * Copyright (c) 2017. The WRENCH Team.
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

#include "workflow_execution_events/WorkflowExecutionEvent.h"
#include "WorkflowFile.h"
#include "WorkflowTask.h"

class WorkflowTask;

namespace wrench {

    /**
     * @brief A workflow abstraction that provides basic functionality to
     * represent/instantiate/manipulate workflows
     */
    class Workflow {

    public:
        Workflow();

        WorkflowTask *addTask(std::string, double, int num_procs = 1);

        void removeTask(WorkflowTask *task);

        WorkflowTask *getWorkflowTaskByID(const std::string);

        WorkflowFile *addFile(const std::string, double);

        WorkflowFile *getWorkflowFileByID(const std::string);

        void addControlDependency(WorkflowTask *, WorkflowTask *);

        void loadFromDAX(const std::string filename);

        unsigned long getNumberOfTasks();

        void exportToEPS(std::string);

        std::set<WorkflowFile *>getInputFiles();

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        bool isDone();

        std::map<std::string, std::vector<WorkflowTask *>> getReadyTasks();

        std::vector<WorkflowTask *> getTasks();

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

        void updateTaskState(WorkflowTask *task, WorkflowTask::State state);

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        std::unique_ptr<lemon::ListDigraph> DAG;  // Lemon DiGraph
        std::unique_ptr<lemon::ListDigraph::NodeMap<WorkflowTask *>> DAG_node_map;  // Lemon map

        std::map<std::string, std::unique_ptr<WorkflowTask>> tasks;
        std::map<std::string, std::unique_ptr<WorkflowFile>> files;

        bool pathExists(WorkflowTask *, WorkflowTask *);

        std::string callback_mailbox;
    };
};

#endif //WRENCH_WORKFLOW_H
