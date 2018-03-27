/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <iostream>
#include "wrench/services/compute/batch/BatchJob.h"
#include "wrench/workflow/WorkflowTask.h"

namespace wrench {
    BatchJob::BatchJob(WorkflowJob *job, unsigned long jobid, unsigned long time_in_minutes, unsigned long num_nodes,
                       unsigned long cores_per_node, double ending_time_stamp, double appeared_time_stamp) {
      if (job == nullptr) {
        throw std::invalid_argument(
                "BatchJob::BatchJob(): StandardJob cannot be null"
        );
      }
      this->job = job;
      if (jobid <= 0 || num_nodes == 0 || cores_per_node == 0) {
        std::cout << "Info: " << jobid << " " << time_in_minutes << " " << num_nodes << " " << cores_per_node << "\n";
        throw std::invalid_argument(
                "BatchJob::BatchJob(): either jobid, time_in_minutes, num_nodes, cores_per_node is less than or equal to zero"
        );
      }
      this->jobid = jobid;
      this->allocated_time = time_in_minutes * 60.0;
      this->num_nodes = num_nodes;
      this->cores_per_node = cores_per_node;
      this->ending_time_stamp = ending_time_stamp;
      this->appeared_time_stamp = appeared_time_stamp;
    }

    unsigned long BatchJob::getAllocatedCoresPerNode() {
      return this->cores_per_node;
    }

    double BatchJob::getAllocatedTime() {
      return this->allocated_time;
    }

    double BatchJob::setAllocatedTime(double time) {
      this->allocated_time = time;
    }

    double BatchJob::getMemoryRequirement() {
      WorkflowJob *workflow_job = this->job;
      double memory_requirement = 0.0;
      if (workflow_job->getType() == WorkflowJob::STANDARD) {
        auto standard_job = (StandardJob *)workflow_job;
        for (auto const &t : standard_job->getTasks()) {
          double ram = t->getMemoryRequirement();
          memory_requirement = (memory_requirement < ram ? ram : memory_requirement);
        }
      }
      return memory_requirement;

    }

    double BatchJob::getAppearedTimeStamp() {
      return this->appeared_time_stamp;
    }

    WorkflowJob *BatchJob::getWorkflowJob() {
      return this->job;
    }

    unsigned long BatchJob::getJobID() {
      return this->jobid;
    }

    unsigned long BatchJob::getNumNodes() {
      return this->num_nodes;
    }

    double BatchJob::getEndingTimeStamp() {
      return this->ending_time_stamp;
    }

    void BatchJob::setEndingTimeStamp(double time_stamp) {
      if (this->ending_time_stamp > 0) {
        throw std::invalid_argument(
                "BatchJob::setEndingTimeStamp(): Cannot set time stamp again for the same job"
        );
      }
      this->ending_time_stamp = time_stamp;
    }

    std::set<std::tuple<std::string, unsigned long, double>> BatchJob::getResourcesAllocated() {
      return this->resources_allocated;
    }

    void BatchJob::setAllocatedResources(std::set<std::tuple<std::string, unsigned long, double>> resources) {
      if (resources.empty()) {
        throw std::invalid_argument(
                "BatchJob::setAllocatedResources(): Empty Resources allocated"
        );
      }
      this->resources_allocated = resources;
    }
}
