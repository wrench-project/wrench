/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <set>

#include "SimplePipelineClustering.h"


namespace wrench {

    /**
     * @brief Main optimization procedure: Group tasks in a pipeline
     *
     * @param workflow: a workflow
     */
    void SimplePipelineClustering::process(Workflow *workflow) {

        int id = 1;
        std::set<std::string> pipelined_tasks;

        for (auto task : workflow->getTasks()) {
            if (pipelined_tasks.find(task->getID()) == pipelined_tasks.end()
                && (task->getNumberOfChildren() == 1 || task->getNumberOfParents() == 1)) {

                std::string cluster_id = "PIPELINE_CLUSTER_" + std::to_string(id++);

                // explore parent
                WorkflowTask *parent = getTask(workflow->getTaskParents(task));
                while (parent && pipelined_tasks.find(parent->getID()) == pipelined_tasks.end()
                       && parent->getNumberOfChildren() == 1) {

                    pipelined_tasks.insert(parent->getID());
                    parent->setClusterID(cluster_id);

                    // next parent
                    parent = getTask(workflow->getTaskParents(parent));
                }

                // explore child
                WorkflowTask *child = getTask(workflow->getTaskChildren(task));
                while (child && pipelined_tasks.find(child->getID()) == pipelined_tasks.end()
                       && child->getNumberOfParents() == 1) {

                    pipelined_tasks.insert(child->getID());
                    child->setClusterID(cluster_id);

                    // next child
                    child = getTask(workflow->getTaskChildren(child));
                }

                // create clustered task
                if (not pipelined_tasks.empty()) {
                    pipelined_tasks.insert(task->getID());
                    task->setClusterID(cluster_id);
                }
            }
        }
    }

    /**
     * @brief Get the first task from a vector of tasks
     *
     * @param tasks: a vector of WorkflowTask
     *
     * @return The first task of the vector
     */
    WorkflowTask *SimplePipelineClustering::getTask(std::vector<WorkflowTask *> tasks) {
        if (tasks.size() <= 1) {
            return not tasks.empty() ? tasks[0] : nullptr;
        }
        return nullptr;
    }
}
