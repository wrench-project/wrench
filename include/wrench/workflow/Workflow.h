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

#include <map>
#include <set>

#include "wrench/execution_events/ExecutionEvent.h"
#include "WorkflowFile.h"
#include "WorkflowTask.h"
#include "DagOfTasks.h"

#include <boost/graph/adjacency_list.hpp>
#include <wrench/workflow/parallel_model/ParallelModel.h>

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
                              double memory_requirement);

        void removeTask(WorkflowTask *task);

        void removeFile(WorkflowFile *file);

        WorkflowTask *getTaskByID(const std::string &id);

        WorkflowFile *addFile(std::string, double);

        WorkflowFile *getFileByID(const std::string &id);

        static double getSumFlops(const std::vector<WorkflowTask *> tasks);

        void addControlDependency(WorkflowTask *src, WorkflowTask *dest, bool redundant_dependencies = false);
        void removeControlDependency(WorkflowTask *src, WorkflowTask *dest);

        unsigned long getNumberOfTasks();

        unsigned long getNumLevels();

        double getCompletionDate();

        void exportToEPS(std::string);

        std::vector<WorkflowFile *> getFiles() const;
        std::map<std::string, WorkflowFile *> getFileMap() const;
        std::vector<WorkflowFile *> getInputFiles() const;
        std::map<std::string, WorkflowFile *> getInputFileMap() const;
        std::vector<WorkflowFile *> getOutputFiles() const;
        std::map<std::string, WorkflowFile *> getOutputFileMap() const;

        std::vector<WorkflowTask *> getTasks();
        std::map<std::string, WorkflowTask *> getTaskMap();
        std::map<std::string, WorkflowTask *> getEntryTaskMap() const;
        std::vector<WorkflowTask *> getEntryTasks() const;
        std::map<std::string, WorkflowTask *> getExitTaskMap() const;
        std::vector<WorkflowTask *> getExitTasks() const;

        std::vector<WorkflowTask *> getTaskParents(const WorkflowTask *task);
        long getTaskNumberOfParents(const  WorkflowTask *task);
        std::vector<WorkflowTask *> getTaskChildren(const WorkflowTask *task);
        long getTaskNumberOfChildren(const WorkflowTask *task);

        bool pathExists(const WorkflowTask *src, const WorkflowTask *dst);


        bool isDone();

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        std::vector<WorkflowTask *> getTasksInTopLevelRange(unsigned long min, unsigned long max);

        std::vector<WorkflowTask *> getReadyTasks();

        std::map<std::string, std::vector<WorkflowTask *>> getReadyClusters();

        /***********************/
        /** \endcond           */
        /***********************/


        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        std::shared_ptr<ExecutionEvent> waitForNextExecutionEvent();
        std::shared_ptr<ExecutionEvent> waitForNextExecutionEvent(double timeout);

        std::string getCallbackMailbox();

//        void updateTaskState(WorkflowTask *task, WorkflowTask::State state);

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        friend class WMS;

        friend class WorkflowTask;

        struct Vertex{ WorkflowTask *task;};
        typedef boost::adjacency_list<boost::listS, boost::vecS, boost::directedS, Vertex> DAG;
        typedef boost::graph_traits<DAG>::vertex_descriptor vertex_t;

        DagOfTasks dag;

        std::map<std::string, std::unique_ptr<WorkflowTask>> tasks;
        std::map<std::string, std::unique_ptr<WorkflowFile>> files;


        std::string callback_mailbox;
        ComputeService *parent_compute_service; // The compute service to which the job was submitted, if any
        Simulation *simulation; // a ptr to the simulation so that the simulation can obtain simulation timestamps for workflow tasks
    };
};

#endif //WRENCH_WORKFLOW_H
