/**
 * Copyright (c) 2017-2019. The WRENCH Team.
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

    class Simulation;

    /**
     * @brief A workflow (to be executed by a WMS)
     */
    class Workflow {

    public:
        Workflow();

        WorkflowTask *addTask(std::string, double flops,
                              unsigned long min_num_cores,
                              unsigned long max_num_cores,
                              double parallel_efficiency,
                              double memory_requirement);

        void removeTask(WorkflowTask *task);

        void removeFile(WorkflowFile *file);

        WorkflowTask *getTaskByID(const std::string &id);

        WorkflowFile *addFile(std::string, double);

        WorkflowFile *getFileByID(const std::string &id);

        static double getSumFlops(std::vector<WorkflowTask *> tasks);

        void addControlDependency(WorkflowTask *src, WorkflowTask *dest, bool redundant_dependencies = false);

//        void loadFromDAX(const std::string &filename,
//                         const std::string &reference_flop_rate,
//                         bool redundant_dependencies = false);
//
//        void loadFromJSON(const std::string &filename,
//                          const std::string &reference_flop_rate,
//                          bool redundant_dependencies = false);
//
//        void loadFromDAXorJSON(const std::string &filename,
//                               const std::string &reference_flop_rate,
//                               bool redundant_dependencies = false);

        unsigned long getNumberOfTasks();

        unsigned long getNumLevels();

        double getCompletionDate();

        void exportToEPS(std::string);

        std::map<std::string, WorkflowFile *> getInputFiles();

        std::map<std::string, WorkflowTask *> getEntryTasks() const;
        std::map<std::string, WorkflowTask *> getExitTasks() const;

        bool isDone();

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        std::vector<WorkflowTask *> getTasksInTopLevelRange(unsigned long min, unsigned long max);

        std::vector<WorkflowTask *> getReadyTasks();

        std::map<std::string, std::vector<WorkflowTask *>> getReadyClusters();

        std::vector<WorkflowTask *> getTasks();

        std::vector<WorkflowFile *> getFiles();

        std::vector<WorkflowTask *> getTaskParents(const WorkflowTask *task);

        std::vector<WorkflowTask *> getTaskChildren(const WorkflowTask *task);

        /***********************/
        /** \endcond           */
        /***********************/


        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        std::shared_ptr<WorkflowExecutionEvent> waitForNextExecutionEvent();
        std::shared_ptr<WorkflowExecutionEvent> waitForNextExecutionEvent(double timeout);

        std::string getCallbackMailbox();

//        void updateTaskState(WorkflowTask *task, WorkflowTask::State state);

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        friend class WMS;

        friend class WorkflowTask;

//        void setNumLevels(unsigned long);

        std::unique_ptr<lemon::ListDigraph> DAG;  // Lemon DiGraph
        std::unique_ptr<lemon::ListDigraph::NodeMap<WorkflowTask *>> DAG_node_map;  // Lemon map

        std::map<std::string, std::unique_ptr<WorkflowTask>> tasks;
        std::map<std::string, std::unique_ptr<WorkflowFile>> files;

//        unsigned long num_levels;

        bool pathExists(WorkflowTask *, WorkflowTask *);

        std::string callback_mailbox;
        ComputeService *parent_compute_service; // The compute service to which the job was submitted, if any
        Simulation *simulation; // a ptr to the simulation so that the simulation can obtain simulation timestamps for workflow tasks
    };
};

#endif //WRENCH_WORKFLOW_H
