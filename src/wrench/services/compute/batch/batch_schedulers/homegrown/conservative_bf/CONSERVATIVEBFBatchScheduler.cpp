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

WRENCH_LOG_NEW_DEFAULT_CATEGORY(conservative_bf_batch_scheduler, "Log category for CONSERVATIVEBFBatchScheduler");

namespace wrench {

    CONSERVATIVEBFBatchScheduler::CONSERVATIVEBFBatchScheduler(BatchComputeService *cs) : HomegrownBatchScheduler(cs) {
        this->schedule = std::unique_ptr<NodeAvailabilityTimeLine>(new NodeAvailabilityTimeLine(cs->total_num_of_nodes));
    }



    void CONSERVATIVEBFBatchScheduler::processJobSubmission(BatchJob *batch_job) {
        WRENCH_INFO("CONSERVATIVE: A new job shows up");

        // Update the time origin
        WRENCH_INFO("CONSERVATIVE: Reset time origin");
        this->schedule->setTimeOrigin((u_int32_t)Simulation::getCurrentSimulatedDate());
//        this->schedule->print();

        // Find its earliest start time
        auto est = this->schedule->findEarliestStartTime(batch_job->getRequestedTime(), batch_job->getRequestedNumNodes());
        WRENCH_INFO("CONSERVATIVE: Earliest start time is: %u", est);

        // Insert it in the schedule
        this->schedule->add(est, est + batch_job->getRequestedTime(), batch_job);
        batch_job->conservative_bf_start_date = est;
        batch_job->conservative_bf_expected_end_date = est + batch_job->getRequestedTime();
//        this->schedule->print();
    }



    void CONSERVATIVEBFBatchScheduler::processQueuedJobs() {
        WRENCH_INFO("CONSERVATIVE: Asked to process queued jobs");
        // Update the time origin
        WRENCH_INFO("CONSERVATIVE: Reset time origin");
        this->schedule->setTimeOrigin((u_int32_t)Simulation::getCurrentSimulatedDate());

//        this->schedule->print();

        // Start  all non-started the jobs in the next slot!
        auto next_jobs = this->schedule->getJobsInFirstSlot();
        for (auto const &batch_job : next_jobs)  {

            WRENCH_INFO("LOOKING AT  BATCH JOB %lu", batch_job->getJobID());

            // If the job has already been allocated resources, it's already running anyway
            if (not batch_job->resources_allocated.empty()) {
                WRENCH_INFO("  Nope, it's already running!");
                continue;
            }

            // Get the workflow job associated to the picked batch job
            WorkflowJob *workflow_job = batch_job->getWorkflowJob();

            // Find on which resources to actually run the job
            unsigned long cores_per_node_asked_for = batch_job->getRequestedCoresPerNode();
            unsigned long num_nodes_asked_for = batch_job->getRequestedNumNodes();
            unsigned long requested_time = batch_job->getRequestedTime();

            WRENCH_INFO("Trying to see if I can run job (batch_job = %lu)(%s)",
                        batch_job->getJobID(),
                        workflow_job->getName().c_str());

//        std::map<std::string, std::tuple<unsigned long, double>> resources;

            auto resources = this->scheduleOnHosts(num_nodes_asked_for, cores_per_node_asked_for, ComputeService::ALL_RAM);
            if (resources.empty()) {
                throw std::runtime_error("Can't run batch job " + std::to_string(batch_job->getJobID()) +  " right now, this shouldn't happen!");
            }

            WRENCH_INFO("Starting job %s (batch job %s)", workflow_job->getName().c_str(), std::to_string(batch_job->getJobID()).c_str());

            // Remove the job from the batch queue
            this->cs->removeJobFromBatchQueue(batch_job);

            // Add it to the running list
            this->cs->running_jobs.insert(batch_job);

            // Start it!
            this->cs->startJob(resources, workflow_job, batch_job, num_nodes_asked_for, requested_time,
                               cores_per_node_asked_for);
        }

        return;
    }



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
            throw std::runtime_error("CONSERVATIVE_BFBatchScheduler::findNextJobToSchedule(): Asking for too many cores per host");
        }


        // IMPORTANT: We always give all cores to a job on a node!
        cores_per_node = Simulation::getHostNumCores(cs->available_nodes_to_cores.begin()->first);

        std::map<std::string, std::tuple<unsigned long, double>> resources = {};
        std::vector<std::string> hosts_assigned = {};

        unsigned long host_count = 0;
        for (auto map_it = cs->available_nodes_to_cores.begin();
             map_it != cs->available_nodes_to_cores.end(); map_it++) {
            if ((*map_it).second >= cores_per_node) {
                //Remove that many cores from the available_nodes_to_core
                (*map_it).second -= cores_per_node;
                hosts_assigned.push_back((*map_it).first);
                resources.insert(std::make_pair((*map_it).first, std::make_tuple(cores_per_node, ram_per_node)));
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

    void CONSERVATIVEBFBatchScheduler::processJobCompletion(BatchJob *batch_job) {
        WRENCH_INFO("CONSERVATIVE: Notified of a JOB COMPLETION!");
        // Update the time origin
        WRENCH_INFO("CONSERVATIVE: Reset time origin");
        auto now = (u_int32_t)Simulation::getCurrentSimulatedDate();
        this->schedule->setTimeOrigin(now);
//        this->schedule->print();

        WRENCH_INFO("CONSERVATIVE: Remove job from schedule entirely");
        // TODO: Is the UINT32_MAX making things slow?
        this->schedule->remove(this->schedule->getTimeOrigin(), UINT32_MAX, batch_job);


        if (now < batch_job->conservative_bf_expected_end_date) {
            WRENCH_INFO("CONSERVATIVE: Batch job %s completed now (%u) but was supposed to complete at time %u... Schedule must be compacted",
                        std::to_string(batch_job->getJobID()).c_str(), now, batch_job->conservative_bf_expected_end_date);
            compactSchedule();
        }

    }

    void CONSERVATIVEBFBatchScheduler::compactSchedule() {

        WRENCH_INFO("CONSERVATIVE_BF: COMPACTING SCHEDULE");
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
            batch_job->conservative_bf_start_date = est + batch_job->getRequestedTime();
        }

//        this->schedule->print();
    }

    void CONSERVATIVEBFBatchScheduler::processJobTermination(BatchJob *batch_job) {
        // Just like a job Completion to me!
        this->processJobCompletion(batch_job);

    }

    void CONSERVATIVEBFBatchScheduler::processJobFailure(BatchJob *batch_job) {
        // Just like a job Completion to me!
        this->processJobCompletion(batch_job);
    }


}
