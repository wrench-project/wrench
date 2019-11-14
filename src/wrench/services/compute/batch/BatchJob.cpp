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
    /**
     * @brief Constructor
     *
     * @param job: the workflow job corresponding to the batch job
     * @param jobid: the batch job id
     * @param time_in_minutes: the requested execution time in minutes
     * @param num_nodes: the requested number of compute nodes (hosts)
     * @param cores_per_node: the requested number of cores per node
     * @param ending_time_stamp: the job's end date
     * @param arrival_time_stamp: the job's arrival date
     */
    BatchJob::BatchJob(WorkflowJob *job, unsigned long jobid, unsigned long time_in_minutes, unsigned long num_nodes,
                       unsigned long cores_per_node, double ending_time_stamp, double arrival_time_stamp) {
        if (job == nullptr) {
            throw std::invalid_argument(
                    "BatchJob::BatchJob(): StandardJob cannot be null"
            );
        }
        this->job = job;
        if (jobid <= 0 || num_nodes == 0 || cores_per_node == 0) {
            throw std::invalid_argument(
                    "BatchJob::BatchJob(): either jobid (" + std::to_string(jobid) +
                    "), time_in_minutes (" + std::to_string(time_in_minutes) +
                    "), num_nodes (" + std::to_string(num_nodes) +
                    "), cores_per_node (" + std::to_string(cores_per_node) +
                    ") is less than or equal to zero"
            );
        }
        this->jobid = jobid;
        this->allocated_time = time_in_minutes * 60;
        this->num_nodes = num_nodes;
        this->cores_per_node = cores_per_node;
        this->ending_time_stamp = ending_time_stamp;
        this->arrival_time_stamp = arrival_time_stamp;

        this->csv_metadata = "color:red";
    }

    /**
     * @brief Get the number of cores per node
     * @return a number of cores
     */
    unsigned long BatchJob::getAllocatedCoresPerNode() {
        return this->cores_per_node;
    }

    /**
     * @brief Get the allocated time
     * @return a time in seconds
     */
    unsigned long BatchJob::getAllocatedTime() {
        return this->allocated_time;
    }

    /**
     * @brief Set the allocated time
     * @param time: a time in seconds
     */
    void BatchJob::setAllocatedTime(unsigned long time) {
        this->allocated_time = time;
    }

    /**
     * @brief Get the memory requirement
     * @return a size in bytes
     */
    double BatchJob::getMemoryRequirement() {
        WorkflowJob *workflow_job = this->job;
        double memory_requirement = 0.0;
        if (workflow_job->getType() == WorkflowJob::STANDARD) {
            auto standard_job = (StandardJob *) workflow_job;
            for (auto const &t : standard_job->getTasks()) {
                double ram = t->getMemoryRequirement();
                memory_requirement = (memory_requirement < ram ? ram : memory_requirement);
            }
        }
        return memory_requirement;

    }

    /**
     * @brief Get the arrival time stamp
     * @return a date
     */
    double BatchJob::getArrivalTimeStamp() {
        return this->arrival_time_stamp;
    }

    /**
     * @brief Get the workflow job corresponding to this batch job
     * @return a workflow job
     */
    WorkflowJob *BatchJob::getWorkflowJob() {
        return this->job;
    }

    /**
     * @brief Get the id of this batch job
     * @return a string id
     */
    unsigned long BatchJob::getJobID() {
        return this->jobid;
    }

    /**
     * @brief Get the number of allocated compute nodes (or hosts)
     * @return a number of nodes
     */
    unsigned long BatchJob::getNumNodes() {
        return this->num_nodes;
    }


    /**
     * @brief Set the batch job's begin timestamp
     * @param time_stamp: a date
     */
    void BatchJob::setBeginTimeStamp(double time_stamp) {
        this->begin_time_stamp = time_stamp;
    }


    /**
     * @brief Get the batch job's begin timestamp
     * @return a date
     */
    double BatchJob::getBeginTimeStamp() {
        return this->begin_time_stamp;
    }

/**
     * @brief Get the batch job's end timestamp
     * @return a date
     */
    double BatchJob::getEndingTimeStamp() {
        return this->ending_time_stamp;
    }

    /**
     * @brief Set the batch job's end timestamp
     * @param time_stamp: a date
     */
    void BatchJob::setEndingTimeStamp(double time_stamp) {
        if (this->ending_time_stamp > 0) {
            throw std::runtime_error(
                    "BatchJob::setEndingTimeStamp(): Cannot set time stamp again for the same job"
            );
        }
        this->ending_time_stamp = time_stamp;
    }

    /**
     * @brief Get the resources allocated to this batch job
     * @return a list of resource, each as a <hostname, number of cores, bytes of RAM> tuple
     */
    std::map<std::string, std::tuple<unsigned long, double>> BatchJob::getResourcesAllocated() {
        return this->resources_allocated;
    }

    /**
     * @brief Set the resources allocated to this batch job
     * @param resources: a list of resource, each as a <hostname, number of cores, bytes of RAM> tuple
     */
    void BatchJob::setAllocatedResources(std::map<std::string, std::tuple<unsigned long, double>> resources) {
        if (resources.empty()) {
            throw std::invalid_argument(
                    "BatchJob::setAllocatedResources(): Empty Resources allocated"
            );
        }
        this->resources_allocated = resources;
    }


}
