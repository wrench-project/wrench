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
#include "wrench/data_file/DataFile.h"
#include "WorkflowTask.h"
#include "DagOfTasks.h"

#include <boost/graph/adjacency_list.hpp>
#include "wrench/workflow/parallel_model/ParallelModel.h"

class WorkflowTask;

namespace wrench {

    class Simulation;


    /**
     * @brief A workflow (to be executed by a WMS)
     */
    class Workflow {

    public:
        Workflow();

        std::shared_ptr<WorkflowTask> addTask(std::string, double flops,
                              unsigned long min_num_cores,
                              unsigned long max_num_cores,
                              double memory_requirement);

        void removeTask(std::shared_ptr<WorkflowTask>task);

        void removeFile(std::shared_ptr<DataFile>file);

        std::shared_ptr<WorkflowTask>getTaskByID(const std::string &id);

        std::shared_ptr<DataFile> addFile(std::string, double);

        std::shared_ptr<DataFile> getFileByID(const std::string &id);

        static double getSumFlops(const std::vector<std::shared_ptr<WorkflowTask>> tasks);

        void addControlDependency(std::shared_ptr<WorkflowTask>src, std::shared_ptr<WorkflowTask>dest, bool redundant_dependencies = false);
        void removeControlDependency(std::shared_ptr<WorkflowTask>src, std::shared_ptr<WorkflowTask>dest);

        unsigned long getNumberOfTasks();

        unsigned long getNumLevels();

        double getCompletionDate();

        void exportToEPS(std::string);

        std::vector<std::shared_ptr<DataFile>> getFiles() const;
        std::map<std::string, std::shared_ptr<DataFile>> getFileMap() const;
        std::vector<std::shared_ptr<DataFile>> getInputFiles() const;
        std::map<std::string, std::shared_ptr<DataFile>> getInputFileMap() const;
        std::vector<std::shared_ptr<DataFile>> getOutputFiles() const;
        std::map<std::string, std::shared_ptr<DataFile>> getOutputFileMap() const;

        std::vector<std::shared_ptr<WorkflowTask>> getTasks();
        std::map<std::string, std::shared_ptr<WorkflowTask>> getTaskMap();
        std::map<std::string, std::shared_ptr<WorkflowTask>> getEntryTaskMap() const;
        std::vector<std::shared_ptr<WorkflowTask>> getEntryTasks() const;
        std::map<std::string, std::shared_ptr<WorkflowTask>> getExitTaskMap() const;
        std::vector<std::shared_ptr<WorkflowTask>> getExitTasks() const;

        std::vector<std::shared_ptr<WorkflowTask>> getTaskParents(const std::shared_ptr<WorkflowTask>task);
        long getTaskNumberOfParents(const  std::shared_ptr<WorkflowTask>task);
        std::vector<std::shared_ptr<WorkflowTask>> getTaskChildren(const std::shared_ptr<WorkflowTask>task);
        long getTaskNumberOfChildren(const std::shared_ptr<WorkflowTask>task);

        bool pathExists(const std::shared_ptr<WorkflowTask>src, const std::shared_ptr<WorkflowTask>dst);

        std::shared_ptr<WorkflowTask> getTaskThatOutputs(std::shared_ptr<DataFile> file);
        bool isFileOutputOfSomeTask(std::shared_ptr<DataFile> file);

        std::set<std::shared_ptr<WorkflowTask>> getTasksThatInput(std::shared_ptr<DataFile> file);
        bool isDone();

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        std::vector<std::shared_ptr<WorkflowTask>> getTasksInTopLevelRange(unsigned long min, unsigned long max);

        std::vector<std::shared_ptr<WorkflowTask>> getReadyTasks();

        std::map<std::string, std::vector<std::shared_ptr<WorkflowTask>>> getReadyClusters();

        /***********************/
        /** \endcond           */
        /***********************/


        /***********************/
        /** \cond INTERNAL     */
        /***********************/


        std::string getCallbackMailbox();

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        friend class WMS;

        friend class WorkflowTask;

        struct Vertex{ std::shared_ptr<WorkflowTask>task;};
        typedef boost::adjacency_list<boost::listS, boost::vecS, boost::directedS, Vertex> DAG;
        typedef boost::graph_traits<DAG>::vertex_descriptor vertex_t;

        DagOfTasks dag;

        /* Map to find tasks by name */
        std::map<std::string, std::shared_ptr<WorkflowTask>> tasks;

        /* Map of output files */
        std::map<std::shared_ptr<DataFile>, std::shared_ptr<WorkflowTask>> task_output_files;
        std::map<std::shared_ptr<DataFile>, std::set<std::shared_ptr<WorkflowTask>>> task_input_files;

        /* Map of files */  // TODO: Move to Simulation
        std::map<std::string, std::shared_ptr<DataFile>> files;

        std::string callback_mailbox;
        ComputeService *parent_compute_service; // The compute service to which the job was submitted, if any
        Simulation *simulation; // a ptr to the simulation so that the simulation can obtain simulation timestamps for workflow tasks
    };
};

#endif //WRENCH_WORKFLOW_H
