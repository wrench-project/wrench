/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <iostream>
#include <wrench/services/compute/batch/BatchJob.h>
#include <wrench/workflow/WorkflowTask.h>

namespace wrench {
    /**
     * @brief Constructor
     *
     * @param job: the compound job corresponding to the BatchComputeService job
     * @param job_id: the BatchComputeService job id
     * @param time_in_minutes: the requested execution time in minutes
     * @param num_nodes: the requested number of compute nodes (hosts)
     * @param cores_per_node: the requested number of cores per node
     * @param username: the username of the user submitting the job
     * @param ending_time_stamp: the job's end date
     * @param arrival_time_stamp: the job's arrival date
     */
    BatchJob::BatchJob(const std::shared_ptr<CompoundJob>& job, unsigned long job_id, unsigned long time_in_minutes, unsigned long num_nodes,
                       unsigned long cores_per_node, const std::string& username, double ending_time_stamp, double arrival_time_stamp) {
        if (job == nullptr) {
            throw std::invalid_argument(
                    "BatchJob::BatchJob(): job cannot be null");
        }
        this->compound_job = job;
        if (job_id <= 0 || num_nodes == 0 || cores_per_node == 0) {
            throw std::invalid_argument(
                    "BatchJob::BatchJob(): either jobid (" + std::to_string(job_id) +
                    "), num_nodes (" + std::to_string(num_nodes) +
                    "), cores_per_node (" + std::to_string(cores_per_node) +
                    ") is less than or equal to zero");
        }

        this->job_id = job_id;
        this->requested_time = time_in_minutes * 60;
        this->requested_num_nodes = num_nodes;
        this->requested_cores_per_node = cores_per_node;
        this->username = username;
        this->ending_time_stamp = ending_time_stamp;
        this->arrival_time_stamp = arrival_time_stamp;

        this->conservative_bf_expected_end_date = 0.0;
        this->conservative_bf_start_date = 0.0;
        this->begin_time_stamp = 0.0;


        this->csv_metadata = "color:red";
    }

    /**
     * @brief Get the requested number of cores per node
     * @return a number of cores
     */
    unsigned long BatchJob::getRequestedCoresPerNode() const {
        return this->requested_cores_per_node;
    }

    /**
     * @brief Get the username
     * @return a username
     */
    std::string BatchJob::getUsername() {
        return this->username;
    }

    /**
     * @brief Get the requested time
     * @return a time in seconds
     */
    unsigned long BatchJob::getRequestedTime() const {
        return this->requested_time;
    }

    /**
     * @brief Set the requested time
     * @param time: a time in seconds
     */
    void BatchJob::setRequestedTime(unsigned long time) {
        this->requested_time = time;
    }

    /**
     * @brief Get the memory_manager_service requirement
     * @return a size in bytes
     */
    double BatchJob::getMemoryRequirement() {
        return this->compound_job->getMinimumRequiredMemory();
    }

    /**
     * @brief Get the arrival time stamp
     * @return a date
     */
    double BatchJob::getArrivalTimestamp() const {
        return this->arrival_time_stamp;
    }

    /**
     * @brief Get the compound job corresponding to this BatchComputeService job
     * @return a compound job
     */
    std::shared_ptr<CompoundJob> BatchJob::getCompoundJob() {
        return this->compound_job;
    }

    /**
     * @brief Get the id of this BatchComputeService job
     * @return a string id
     */
    unsigned long BatchJob::getJobID() const {
        return this->job_id;
    }

    /**
     * @brief Get the number of requested compute nodes (or hosts)
     * @return a number of nodes
     */
    unsigned long BatchJob::getRequestedNumNodes() const {
        return this->requested_num_nodes;
    }


    /**
     * @brief Set the BatchComputeService job's begin timestamp
     * @param time_stamp: a date
     */
    void BatchJob::setBeginTimestamp(double time_stamp) {
        this->begin_time_stamp = time_stamp;
    }


    /**
     * @brief Get the BatchComputeService job's begin timestamp
     * @return a date
     */
    double BatchJob::getBeginTimestamp() const {
        return this->begin_time_stamp;
    }

    /**
     * @brief Get the BatchComputeService job's end timestamp
     * @return a date
     */
    double BatchJob::getEndingTimestamp() const {
        return this->ending_time_stamp;
    }

    /**
     * @brief Set the BatchComputeService job's end timestamp
     * @param time_stamp: a date
     */
    void BatchJob::setEndingTimestamp(double time_stamp) {
        if (this->ending_time_stamp > 0) {
            throw std::runtime_error(
                    "BatchJob::setEndingTimestamp(): Cannot set time stamp again for the same job");
        }
        this->ending_time_stamp = time_stamp;
    }

    /**
     * @brief Get the resources allocated to this BatchComputeService job
     * @return a list of resource, each as a <hostname, number of cores, bytes of RAM> tuple
     */
    std::map<std::string, std::tuple<unsigned long, double>> BatchJob::getResourcesAllocated() {
        return this->resources_allocated;
    }

    /**
     * @brief Set the resources allocated to this BatchComputeService job
     * @param resources: a list of resource, each as a <hostname, number of cores, bytes of RAM> tuple
     */
    void BatchJob::setAllocatedResources(const std::map<std::string, std::tuple<unsigned long, double>>& resources) {
        if (resources.empty()) {
            throw std::invalid_argument(
                    "BatchJob::setAllocatedResources(): Empty Resources allocated");
        }
        this->resources_allocated = resources;
    }


}// namespace wrench
