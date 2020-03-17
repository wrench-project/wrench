/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/logging/TerminalOutput.h"
#include "wrench/simulation/Simulation.h"

#include "CONSERVATIVEBFBatchScheduler.h"

//#define  PRINT_SCHEDULE 1

WRENCH_LOG_NEW_DEFAULT_CATEGORY(conservative_bf_batch_scheduler, "Log category for CONSERVATIVEBFBatchScheduler");

namespace wrench {

    /**
     * @brief Constructor
     * @param cs: The BatchComputeService for which this scheduler is working
     */
    CONSERVATIVEBFBatchScheduler::CONSERVATIVEBFBatchScheduler(BatchComputeService *cs) : HomegrownBatchScheduler(cs) {
        this->schedule = std::unique_ptr<NodeAvailabilityTimeLine>(new NodeAvailabilityTimeLine(cs->total_num_of_nodes));
    }

    /**
     * @brief Method to process a job submission
     * @param batch_job: the newly submitted batch job
     */
    void CONSERVATIVEBFBatchScheduler::processJobSubmission(std::shared_ptr<BatchJob> batch_job) {

        WRENCH_INFO("Scheduling a new batch job, %lu, that needs %lu nodes",
                    batch_job->getJobID(),  batch_job->getRequestedNumNodes());

        // Update the time origin
        this->schedule->setTimeOrigin((u_int32_t)Simulation::getCurrentSimulatedDate());

        // Find its earliest possible start time
        auto est = this->schedule->findEarliestStartTime(batch_job->getRequestedTime(), batch_job->getRequestedNumNodes());
//        WRENCH_INFO("The Earliest start time is: %u", est);

        // Insert it in the schedule
        this->schedule->add(est, est + batch_job->getRequestedTime(), batch_job);
        batch_job->conservative_bf_start_date = est;
        batch_job->conservative_bf_expected_end_date = est + batch_job->getRequestedTime();
        WRENCH_INFO("Scheduled batch job %lu on %lu from time %u to %u",
                    batch_job->getJobID(), batch_job->getRequestedNumNodes(),
                    batch_job->conservative_bf_start_date, batch_job->conservative_bf_expected_end_date);
#ifdef PRINT_SCHEDULE
        this->schedule->print();
#endif
    }

    /**
     * @brief Method to schedule (possibly) the next jobs to be scheduled
     */
    void CONSERVATIVEBFBatchScheduler::processQueuedJobs() {

        if (this->cs->batch_queue.empty()){
            return;
        }

        // Update the time origin
        this->schedule->setTimeOrigin((u_int32_t)Simulation::getCurrentSimulatedDate());

        // Start  all non-started the jobs in the next slot!
        auto next_jobs = this->schedule->getJobsInFirstSlot();

        if (next_jobs.empty()) {
            std::cerr <<  "this-.cs->batch_queue has " << this->cs->batch_queue.size() << "jobs\n";
            for (auto const &j : this->cs->batch_queue) {
                std::cerr << "    - " << j->getJobID() << "\n";
            }
            throw  std::runtime_error("CONSERVATIVEBFBatchScheduler::processQueuedJobs(): next_jobs is empty. This shouldn't happen");
        }

        for (auto const &batch_job : next_jobs)  {

            // If the job has already been allocated resources, it's already running anyway
            if (not batch_job->resources_allocated.empty()) {
                continue;
            }

            // Get the workflow job associated to the picked batch job
            WorkflowJob *workflow_job = batch_job->getWorkflowJob();

            // Find on which resources to actually run the job
            unsigned long cores_per_node_asked_for = batch_job->getRequestedCoresPerNode();
            unsigned long num_nodes_asked_for = batch_job->getRequestedNumNodes();
            unsigned long requested_time = batch_job->getRequestedTime();

            auto resources = this->scheduleOnHosts(num_nodes_asked_for, cores_per_node_asked_for, ComputeService::ALL_RAM);
            if (resources.empty()) {
                // Hmmm... we don't have the resources right now... we should get an update soon....
                return;
//                throw std::runtime_error("Can't run batch job " + std::to_string(batch_job->getJobID()) +  " right now, this shouldn't happen!");
            }

            WRENCH_INFO("Starting batch job %lu ", batch_job->getJobID());

            // Remove the job from the batch queue
            this->cs->removeJobFromBatchQueue(batch_job);

            // Add it to the running list
            this->cs->running_jobs.insert(batch_job);

            // Start it!
            this->cs->startJob(resources, workflow_job, batch_job, num_nodes_asked_for, requested_time,
                               cores_per_node_asked_for);
        }
    }

    /**
     * @brief Method to compact the schedule
     */
    void CONSERVATIVEBFBatchScheduler::compactSchedule() {

        WRENCH_INFO("Compacting schedule...");

        // For each job in the order of the batch queue:
        //   - remove the job from the schedule
        //   - re-insert it as early as possible

        // Reset the time origin
        auto now = (u_int32_t)Simulation::getCurrentSimulatedDate();
        this->schedule->setTimeOrigin(now);

        // Go through the batch queue
        for (auto const &batch_job : this->cs->batch_queue) {

            // Remove the job from the schedule
            this->schedule->remove(batch_job->conservative_bf_start_date, batch_job->conservative_bf_expected_end_date + 100, batch_job);

            // Find the earliest start time
            auto est = this->schedule->findEarliestStartTime(batch_job->getRequestedTime(), batch_job->getRequestedNumNodes());

            // Insert it in the schedule
            this->schedule->add(est, est + batch_job->getRequestedTime(), batch_job);

            batch_job->conservative_bf_start_date = est;
            batch_job->conservative_bf_expected_end_date = est + batch_job->getRequestedTime();
        }


#if 0 // OLD IMPLEMENTATION THAT RECONSTRUCTS THE SCHEDULE FROM SCRATCH
        // Clear the schedule
        this->schedule->clear();

        // Reset the time origin
        auto now = (u_int32_t)Simulation::getCurrentSimulatedDate();
        this->schedule->setTimeOrigin(now);

        // Add the running job time slots
        for (auto  const &batch_job : this->cs->running_jobs) {
            this->schedule->add(now, batch_job->conservative_bf_expected_end_date, batch_job);
        }

        // Add in all other jobs as early as possible in batch queue order
        for (auto const &batch_job : this->cs->batch_queue) {
            auto est = this->schedule->findEarliestStartTime(batch_job->getRequestedTime(), batch_job->getRequestedNumNodes());
            // Insert it in the schedule
            this->schedule->add(est, est + batch_job->getRequestedTime(), batch_job);
            batch_job->conservative_bf_start_date = est;
            batch_job->conservative_bf_expected_end_date = est + batch_job->getRequestedTime();
        }
#endif

#ifdef PRINT_SCHEDULE
        this->schedule->print();
#endif
    }

    /**
     * @brief Method to process a job completion
     * @param batch_job: the job that completed
     */
    void CONSERVATIVEBFBatchScheduler::processJobCompletion(std::shared_ptr<BatchJob> batch_job) {
        WRENCH_INFO("Notified of completion of batch job, %lu", batch_job->getJobID());

        auto now = (u_int32_t)Simulation::getCurrentSimulatedDate();
        this->schedule->setTimeOrigin(now);
        this->schedule->remove(now, batch_job->conservative_bf_expected_end_date + 100, batch_job);

#ifdef PRINT_SCHEDULE
        this->schedule->print();
#endif

        if (now < batch_job->conservative_bf_expected_end_date) {
            compactSchedule();
        }
    }

    /**
    * @brief Method to process a job termination
    * @param batch_job: the job that was terminated
    */
    void CONSERVATIVEBFBatchScheduler::processJobTermination(std::shared_ptr<BatchJob> batch_job) {
        // Just like a job Completion to me!
        this->processJobCompletion(batch_job);
    }

    /**
    * @brief Method to process a job failure
    * @param batch_job: the job that failed
    */
    void CONSERVATIVEBFBatchScheduler::processJobFailure(std::shared_ptr<BatchJob> batch_job) {
        // Just like a job Completion to me!
        this->processJobCompletion(batch_job);
    }

    /**
     * @brief Method to figure out on which actual resources a job could be scheduled right now
     * @param num_nodes: number of nodes
     * @param cores_per_node: number of cores per node
     * @param ram_per_node: amount of RAM
     * @return
     */
    std::map<std::string, std::tuple<unsigned long, double>>
    CONSERVATIVEBFBatchScheduler::scheduleOnHosts(unsigned long num_nodes, unsigned long cores_per_node, double ram_per_node) {

        if (ram_per_node == ComputeService::ALL_RAM) {
            ram_per_node = Simulation::getHostMemoryCapacity(cs->available_nodes_to_cores.begin()->first);
        }
        if (cores_per_node == ComputeService::ALL_CORES) {
            cores_per_node = Simulation::getHostNumCores(cs->available_nodes_to_cores.begin()->first);
        }

        if (ram_per_node > Simulation::getHostMemoryCapacity(cs->available_nodes_to_cores.begin()->first)) {
            throw std::runtime_error("CONSERVATIVE_BFBatchScheduler::findNextJobToSchedule(): Asking for too much RAM per host");
        }
        if (num_nodes > cs->available_nodes_to_cores.size()) {
            throw std::runtime_error("CONSERVATIVE_BFBatchScheduler::findNextJobToSchedule(): Asking for too many hosts");
        }
        if (cores_per_node > Simulation::getHostNumCores(cs->available_nodes_to_cores.begin()->first)) {
            throw std::runtime_error("CONSERVATIVE_BFBatchScheduler::findNextJobToSchedule(): Asking for too many cores per host (asking  for " +
                                     std::to_string(cores_per_node) + " but hosts have " +
                                     std::to_string(Simulation::getHostNumCores(cs->available_nodes_to_cores.begin()->first)) + "cores)");
        }

        // IMPORTANT: We always give all cores to a job on a node!
        cores_per_node = Simulation::getHostNumCores(cs->available_nodes_to_cores.begin()->first);

        std::map<std::string, std::tuple<unsigned long, double>> resources = {};
        std::vector<std::string> hosts_assigned = {};

        unsigned long host_count = 0;
        for (auto & available_nodes_to_core : cs->available_nodes_to_cores) {
            if (available_nodes_to_core.second >= cores_per_node) {
                //Remove that many cores from the available_nodes_to_core
                available_nodes_to_core.second -= cores_per_node;
                hosts_assigned.push_back(available_nodes_to_core.first);
                resources.insert(std::make_pair(available_nodes_to_core.first, std::make_tuple(cores_per_node, ram_per_node)));
                if (++host_count >= num_nodes) {
                    break;
                }
            }
        }
        if (resources.size() < num_nodes) {
            resources = {};
            std::vector<std::string>::iterator vector_it;
            // undo!
            for (vector_it = hosts_assigned.begin(); vector_it != hosts_assigned.end(); vector_it++) {
                cs->available_nodes_to_cores[*vector_it] += cores_per_node;
            }
        }

        return resources;
    }

    /**
     * @brief Method to obtain start time estimates
     * @param set_of_jobs: a set of job specs
     * @return map of estimates
     */
    std::map<std::string, double> CONSERVATIVEBFBatchScheduler::getStartTimeEstimates(
            std::set<std::tuple<std::string, unsigned long, unsigned long, double>> set_of_jobs) {
        std::map<std::string, double> to_return;

        for (auto const &j : set_of_jobs) {
            const std::string& id = std::get<0>(j);
            u_int64_t num_nodes = std::get<1>(j);
            u_int64_t num_cores_per_host = this->cs->num_cores_per_node;  // Ignore this one. Assume all  cores!
            if (std::get<3>(j) > UINT32_MAX) {
                throw std::runtime_error("CONSERVATIVEBFBatchScheduler::getStartTimeEstimates(): job duration too large");
            }
            auto duration = (u_int32_t)(std::get<3>(j));

            auto est = this->schedule->findEarliestStartTime(duration, num_nodes);
            if (est <  UINT32_MAX) {
                to_return[id] = (double) est;
            } else {
                to_return[id] = -1.0;
            }
        }
        return  to_return;
    }

}
