/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/logging/TerminalOutput.h>
#include <wrench/simulation/Simulation.h>

#include <memory>

#include "wrench/services/compute/batch/batch_schedulers/homegrown/easy_bf/EasyBackfillingBatchScheduler.h"

//#define  PRINT_SCHEDULE 1

WRENCH_LOG_CATEGORY(wrench_core_easy_bf_batch_scheduler, "Log category for EasyBackfillingBatchScheduler");

namespace wrench {

    /**
     * @brief Constructor
     * @param cs: The BatchComputeService for which this scheduler is working
     */
    EasyBackfillingBatchScheduler::EasyBackfillingBatchScheduler(BatchComputeService *cs, int depth) : HomegrownBatchScheduler(cs) {
        this->schedule = std::make_unique<NodeAvailabilityTimeLine>(cs->total_num_of_nodes);
        this->_depth = depth;
    }

    /**
     * @brief Method to process a job submission
     * @param batch_job: the newly submitted BatchComputeService job
     */
    void EasyBackfillingBatchScheduler::processJobSubmission(std::shared_ptr<BatchJob> batch_job) {
        WRENCH_INFO("Scheduling a new BatchComputeService job, %lu, that needs %lu nodes",
                    batch_job->getJobID(), batch_job->getRequestedNumNodes());

//        std::cerr << "NEW JOB: " << batch_job->getCompoundJob()->getName() << " (DOING NOTHING)\n";
    }

    /**
     * @brief Method to schedule (possibly) the next jobs to be scheduled
     */
    void EasyBackfillingBatchScheduler::processQueuedJobs() {
        if (this->cs->batch_queue.empty()) {
            return;
        }

        // Update the time origin
        double now = Simulation::getCurrentSimulatedDate();
        std::cerr << "** [" <<  now << "] IN PROCESSING QUEUE JOB (" << this->cs->batch_queue.size() << " JOBS IN THE QUEUE)" << std::endl;
        this->schedule->setTimeOrigin((u_int32_t) now);

        // While the first job can be scheduled now, schedule it
        unsigned int i;
        for (i = 0; i < this->cs->batch_queue.size(); i++) {
            auto first_job = this->cs->batch_queue.at(i);
            // If the job has already been allocated resources, it's already running anyway
            if (not first_job->resources_allocated.empty()) {
                continue;
            }
            auto earliest_start_time = this->schedule->findEarliestStartTime(first_job->requested_time, first_job->getRequestedNumNodes());
            std::cerr << "  LOOKING AT JOB " << first_job->getCompoundJob()->getName() << ": IT HAS EARLIEST START TIME " << earliest_start_time << "\n";
            // If the earliest start time is more than a second from now, never mind
            if (std::abs(earliest_start_time - now) > 1) {
                std::cerr << "  CAN'T SCHEDULE IT NOW, OH WELL\n";
                break;
            }
            std::cerr << "  SCHEDULING JOB FOR START: " << first_job->getCompoundJob()->getName() << "\n";
            // Insert the job into the schedule
            this->schedule->add(earliest_start_time, earliest_start_time + first_job->getRequestedTime(), first_job);
            first_job->easy_bf_start_date = earliest_start_time;
            first_job->easy_bf_expected_end_date = earliest_start_time + first_job->getRequestedTime();
            WRENCH_INFO("Scheduled BatchComputeService job %lu on %lu nodes from time %u to %u",
                        first_job->getJobID(), first_job->getRequestedNumNodes(),
                        first_job->easy_bf_start_date, first_job->easy_bf_expected_end_date);
        }
        unsigned int first_job_not_started = i;

        if (first_job_not_started < this->cs->batch_queue.size()) {

            // At this point, the first job in the queue cannot start now, so determine when it could start
            auto first_job_start_time = this->schedule->findEarliestStartTime(
                    this->cs->batch_queue.at(first_job_not_started)->requested_time,
                    this->cs->batch_queue.at(first_job_not_started)->getRequestedNumNodes());

            std::cerr << "THE FIRST JOB'S (" << this->cs->batch_queue.at(first_job_not_started)->getCompoundJob()->getName() << ") GUARANTEED START TIME IS: " << first_job_start_time << "\n";

            // Go through all the other jobs, and start each one that can start
            // (without hurting the first job in the queue if the depth is 1)
            for (unsigned int i = first_job_not_started + 1; i < this->cs->batch_queue.size(); i++) {
                auto candidate_job = this->cs->batch_queue.at(i);

                if (not candidate_job->resources_allocated.empty()) {
                    continue;
                }
                auto earliest_start_time = this->schedule->findEarliestStartTime(
                        candidate_job->requested_time,
                        candidate_job->getRequestedNumNodes());

                // If the job cannot start now, nevermind
                if (std::abs(earliest_start_time - now) > 1) {
                    continue;
                }

                // Tentatively add the job to the schedule
                if (this->_depth == 0) {
                    this->schedule->add(earliest_start_time, earliest_start_time + candidate_job->getRequestedTime(),
                                        candidate_job);
                        std::cerr << "BACKFILL_D0: SCHEDULING JOB FOR START: " << candidate_job->getCompoundJob()->getName() << "\n";
                } else if (this->_depth == 1) {
                    this->schedule->add(earliest_start_time, earliest_start_time + candidate_job->getRequestedTime(),
                                        candidate_job);
                    // Check whether starting the job now would postpone the first job in the queue
                    auto new_first_job_start_time = this->schedule->findEarliestStartTime(
                            this->cs->batch_queue.at(first_job_not_started)->requested_time,
                            this->cs->batch_queue.at(first_job_not_started)->getRequestedNumNodes());
                    // If the first job would be harmed, remove the tentative job from the schedule
                    std::cerr << "BACKFILL? OLD=" << first_job_start_time << "  NEW=" << new_first_job_start_time << "\n";
                    if (new_first_job_start_time > first_job_start_time) {
                        this->schedule->remove(earliest_start_time,
                                               earliest_start_time + candidate_job->getRequestedTime(),
                                               candidate_job);
                    } else {
                        std::cerr << "BACKFILL_D1: SCHEDULING JOB FOR START: " << candidate_job->getCompoundJob()->getName() << " AT TIME " << earliest_start_time << "\n";
                    }
                }
            }
        }

        std::cerr << "STARTING ALL THE JOBS THAT WERE SCHEDULED, GIVEN THIS SCHEDULE\n";
        this->schedule->print();

        // Start all non-started the jobs in the next slot!
//        std::cerr << "GETTING THE JOBS IN THE NEXT SLOT\n";
        std::set<std::shared_ptr<BatchJob>> next_jobs = this->schedule->getJobsInFirstSlot();
        if (next_jobs.empty()) {
            this->compactSchedule();
            next_jobs = this->schedule->getJobsInFirstSlot();
        }

        for (auto const &batch_job: next_jobs) {
            std::cerr << "   --> " << batch_job->getCompoundJob()->getName() << "\n";
            // If the job has already been allocated resources, it's already running anyway
            if (not batch_job->resources_allocated.empty()) {
                continue;
            }

            // Get the workflow job associated to the picked BatchComputeService job
            std::shared_ptr<CompoundJob> compound_job = batch_job->getCompoundJob();

            // Find on which resources to actually run the job
            unsigned long cores_per_node_asked_for = batch_job->getRequestedCoresPerNode();
            unsigned long num_nodes_asked_for = batch_job->getRequestedNumNodes();
            unsigned long requested_time = batch_job->getRequestedTime();

            auto resources = this->scheduleOnHosts(num_nodes_asked_for, cores_per_node_asked_for, ComputeService::ALL_RAM);
            if (resources.empty()) {
                std::cerr << "HMMM... DID NOT FIND RESOURCES\n";
                // Hmmm... we don't have the resources right now... we should get an update soon....
                return;
                //                throw std::runtime_error("Can't run BatchComputeService job " + std::to_string(batch_job->getJobID()) +  " right now, this shouldn't happen!");
            }

            WRENCH_INFO("Starting BatchComputeService job %lu ", batch_job->getJobID());
            std::cerr << "STARTING JOB " << batch_job->getCompoundJob()->getName() << "\n";

            // Remove the job from the BatchComputeService queue
            this->cs->removeJobFromBatchQueue(batch_job);

            // Add it to the running list
            this->cs->running_jobs[batch_job->getCompoundJob()] = batch_job;

            // Start it!
            this->cs->startJob(resources, compound_job, batch_job, num_nodes_asked_for, requested_time,
                               cores_per_node_asked_for);
        }
    }

    /**
     * @brief Method to compact the schedule
     */
    // THIS METHOD IS LIKELY USELESS FOR EASY......
    void EasyBackfillingBatchScheduler::compactSchedule() {
        return;
        WRENCH_INFO("Compacting schedule...");

        std::cerr << "IN COMPACT SCHEDULE\n";

#ifdef PRINT_SCHEDULE
        //        WRENCH_INFO("BEFORE COMPACTING");
        this->schedule->print();
#endif

        // For each job in the order of the BatchComputeService queue:
        //   - remove the job from the schedule
        //   - re-insert it as early as possible

        // Reset the time origin
        auto now = (u_int32_t) Simulation::getCurrentSimulatedDate();
        this->schedule->setTimeOrigin(now);

        // Go through the BatchComputeService queue
        for (auto const &batch_job: this->cs->batch_queue) {
            //            WRENCH_INFO("DEALING WITH JOB %lu", batch_job->getJobID());

            // Remove the job from the schedule
            //            WRENCH_INFO("REMOVING IT FROM SCHEDULE");
            this->schedule->remove(batch_job->easy_bf_start_date, batch_job->easy_bf_expected_end_date + 100, batch_job);
            //            this->schedule->print();

            // Find the earliest start time
            //            WRENCH_INFO("FINDING THE EARLIEST START TIME");
            auto est = this->schedule->findEarliestStartTime(batch_job->getRequestedTime(), batch_job->getRequestedNumNodes());
            //            WRENCH_INFO("EARLIEST START TIME FOR IT: %u", est);
            // Insert it in the schedule
            this->schedule->add(est, est + batch_job->getRequestedTime(), batch_job);
            //            WRENCH_INFO("RE-INSERTED THERE!");
            //            this->schedule->print();

            batch_job->easy_bf_start_date = est;
            batch_job->easy_bf_expected_end_date = est + batch_job->getRequestedTime();
        }




#ifdef PRINT_SCHEDULE
        WRENCH_INFO("AFTER COMPACTING");
        this->schedule->print();
#endif
    }

    /**
     * @brief Method to process a job completion
     * @param batch_job: the job that completed
     */
    void EasyBackfillingBatchScheduler::processJobCompletion(std::shared_ptr<BatchJob> batch_job) {
        WRENCH_INFO("Notified of completion of BatchComputeService job, %lu", batch_job->getJobID());

        auto now = (u_int32_t) Simulation::getCurrentSimulatedDate();
        this->schedule->setTimeOrigin(now);
        this->schedule->remove(now, batch_job->easy_bf_expected_end_date + 100, batch_job);

#ifdef PRINT_SCHEDULE
        this->schedule->print();
#endif

        // TODO: REMOVE THIS, AS FOR EASY IT LIKELY DOESN'T DO ANYTHING
        if (now < batch_job->easy_bf_expected_end_date) {
            compactSchedule();
        }
    }

    /**
    * @brief Method to process a job termination
    * @param batch_job: the job that was terminated
    */
    void EasyBackfillingBatchScheduler::processJobTermination(std::shared_ptr<BatchJob> batch_job) {
        // Just like a job Completion to me!
        this->processJobCompletion(batch_job);
    }

    /**
    * @brief Method to process a job failure
    * @param batch_job: the job that failed
    */
    void EasyBackfillingBatchScheduler::processJobFailure(std::shared_ptr<BatchJob> batch_job) {
        // Just like a job Completion to me!
        this->processJobCompletion(batch_job);
    }

    /**
     * @brief Method to figure out on which actual resources a job could be scheduled right now
     * @param num_nodes: number of nodes
     * @param cores_per_node: number of cores per node
     * @param ram_per_node: amount of RAM
     * @return a host:<core,RAM> map
     *
     */
    std::map<simgrid::s4u::Host *, std::tuple<unsigned long, sg_size_t>>
    EasyBackfillingBatchScheduler::scheduleOnHosts(unsigned long num_nodes, unsigned long cores_per_node, sg_size_t ram_per_node) {
        if (ram_per_node == ComputeService::ALL_RAM) {
            ram_per_node = S4U_Simulation::getHostMemoryCapacity(cs->available_nodes_to_cores.begin()->first);
        }
        if (cores_per_node == ComputeService::ALL_CORES) {
            cores_per_node = cs->available_nodes_to_cores.begin()->first->get_core_count();
        }

        if (ram_per_node > S4U_Simulation::getHostMemoryCapacity(cs->available_nodes_to_cores.begin()->first)) {
            throw std::runtime_error("CONSERVATIVE_BFBatchScheduler::scheduleOnHosts(): Asking for too much RAM per host");
        }
        if (num_nodes > cs->available_nodes_to_cores.size()) {
            throw std::runtime_error("CONSERVATIVE_BFBatchScheduler::scheduleOnHosts(): Asking for too many hosts");
        }
        if (cores_per_node > (unsigned long) cs->available_nodes_to_cores.begin()->first->get_core_count()) {
            throw std::runtime_error("CONSERVATIVE_BFBatchScheduler::scheduleOnHosts(): Asking for too many cores per host (asking  for " +
                                     std::to_string(cores_per_node) + " but hosts have " +
                                     std::to_string(cs->available_nodes_to_cores.begin()->first->get_core_count()) + "cores)");
        }

        // IMPORTANT: We always give all cores to a job on a node!
        cores_per_node = cs->available_nodes_to_cores.begin()->first->get_core_count();

        return HomegrownBatchScheduler::selectHostsFirstFit(cs, num_nodes, cores_per_node, ram_per_node);
    }

    /**
     * @brief Method to obtain start time estimates
     * @param set_of_jobs: a set of job specs
     * @return map of estimates
     */
    std::map<std::string, double> EasyBackfillingBatchScheduler::getStartTimeEstimates(
            std::set<std::tuple<std::string, unsigned long, unsigned long, sg_size_t>> set_of_jobs) {
        std::map<std::string, double> to_return;

        throw std::runtime_error("EasyBackfillingBatchScheduler::getStartTimeEstimates(): Method not implemented (ever?) for EASY backfilling");
    }

}// namespace wrench
