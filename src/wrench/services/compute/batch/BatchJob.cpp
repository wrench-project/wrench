//
// Created by suraj on 9/16/17.
//

#include <iostream>
#include "wrench/services/compute/batch/BatchJob.h"

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
      this->time_in_minutes = time_in_minutes;
      this->num_nodes = num_nodes;
      this->cores_per_node = cores_per_node;
      this->ending_time_stamp = ending_time_stamp;
      this->appeared_time_stamp = appeared_time_stamp;
    }

    unsigned long BatchJob::getAllocatedCoresPerNode() {
      return this->cores_per_node;
    }

    unsigned long BatchJob::getAllocatedTime() {
      return this->time_in_minutes;
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
