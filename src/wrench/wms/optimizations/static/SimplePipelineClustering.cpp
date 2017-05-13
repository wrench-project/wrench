/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief wrench::StaticSimpleClustering implements a simple task
 *  clustering based on the max number of tasks per cluster
 */

#include "wms/optimizations/static/SimplePipelineClustering.h"
#include <set>

namespace wrench {

    /**
     * @brief Main function for static optimizations: Group tasks in a pipeline
     *
     * @param workflow: a pointer to a Workflow object
     */
    void SimplePipelineClustering::process(Workflow *workflow) {

      // TODO: Refactor to avoid code duplication

      int id = 1;
      std::set<std::string> pipelined_tasks;

      for (auto task : workflow->getTasks()) {
        if (task->getNumberOfChildren() == 1 && task->getNumberOfParents() == 1 &&
            pipelined_tasks.find(task->getId()) == pipelined_tasks.end()) {

          std::string cluster_id = "PIPELINE_CLUSTER_" + std::to_string(id++);

          // explore parent
          WorkflowTask *parent = workflow->getTaskParents(task)[0];
          while (true) {
            if (parent->getNumberOfChildren() == 1 && parent->getNumberOfParents() == 1 &&
                pipelined_tasks.find(parent->getId()) == pipelined_tasks.end()) {

              pipelined_tasks.insert(parent->getId());
              parent->setClusterId(cluster_id);
              std::cout << "CALLING READY" << std::endl;
              parent->setReady();

              // next parent
              parent = workflow->getTaskParents(parent)[0];

            } else {
              break;
            }
          }

          // explore child
          WorkflowTask *child = workflow->getTaskChildren(task)[0];
          while (true) {
            if (child->getNumberOfChildren() == 1 && child->getNumberOfParents() == 1 &&
                pipelined_tasks.find(child->getId()) == pipelined_tasks.end()) {

              pipelined_tasks.insert(child->getId());
              child->setClusterId(cluster_id);
              std::cout << "CALLING READY (C)" << std::endl;
              child->setReady();

              // next child
              child = workflow->getTaskChildren(child)[0];

            } else {
              break;
            }
          }

          // create clustered task
          if (not pipelined_tasks.empty()) {
            pipelined_tasks.insert(task->getId());
            task->setClusterId(cluster_id);
          }
        }
      }
    }
}
