/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/batch/batch_schedulers/homegrown/fcfs/FCFSBatchScheduler.h"
#include <wrench/simulation/Simulation.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/compute/batch/BatchComputeService.h>

WRENCH_LOG_CATEGORY(wrench_core_fcfs_batch_scheduler, "Log category for FCFSBatchScheduler");

namespace wrench {

    /**
    * @brief Overridden Method to pick the next job to schedule
    *
    * @return A BatchComputeService job, or nullptr is none is found
    */
    std::shared_ptr<BatchJob> FCFSBatchScheduler::pickNextJobToSchedule() const
    {
        if (this->cs->batch_queue.empty()) {
            return nullptr;
        } else {
            return *this->cs->batch_queue.begin();
        }
    }


    /**
     * @brief Override Method to find hosts on which to schedule a  job
     * @param num_nodes: the job's requested num nodes
     * @param cores_per_node: the job's num cores per node
     * @param ram_per_node: the job's ram per node
     * @return A resource list
     */
    std::map<simgrid::s4u::Host *, std::tuple<unsigned long, sg_size_t>> FCFSBatchScheduler::scheduleOnHosts(
            unsigned long num_nodes, unsigned long cores_per_node, sg_size_t ram_per_node) {

        if (ram_per_node == ComputeService::ALL_RAM) {
            ram_per_node = S4U_Simulation::getHostMemoryCapacity(cs->available_nodes_to_cores.begin()->first);
        }
        if (cores_per_node == ComputeService::ALL_CORES) {
            cores_per_node = cs->available_nodes_to_cores.begin()->first->get_core_count();
        }

        if (ram_per_node > S4U_Simulation::getHostMemoryCapacity(cs->available_nodes_to_cores.begin()->first)) {
            throw std::runtime_error("FCFSBatchScheduler::scheduleOnHosts(): Asking for too much RAM per host");
        }
        if (num_nodes > cs->available_nodes_to_cores.size()) {
            throw std::runtime_error("FCFSBatchScheduler::scheduleOnHosts(): Asking for too many hosts");
        }
        if (cores_per_node > static_cast<unsigned long>(cs->available_nodes_to_cores.begin()->first->get_core_count())) {
            throw std::runtime_error("FCFSBatchScheduler::scheduleOnHosts(): Asking for too many cores per host");
        }

        auto host_selection_algorithm = this->cs->getPropertyValueAsString(BatchComputeServiceProperty::HOST_SELECTION_ALGORITHM);

        if (host_selection_algorithm == "FIRSTFIT") {
            return HomegrownBatchScheduler::selectHostsFirstFit(cs, num_nodes, cores_per_node, ram_per_node);
        } else if (host_selection_algorithm == "BESTFIT") {
            return HomegrownBatchScheduler::selectHostsBestFit(cs, num_nodes, cores_per_node, ram_per_node);

        } else if (host_selection_algorithm == "ROUNDROBIN") {
            static unsigned long round_robin_host_selector_idx = -1;
            return HomegrownBatchScheduler::selectHostsRoundRobin(cs, &round_robin_host_selector_idx, num_nodes, cores_per_node, ram_per_node);
        } else {
            throw std::invalid_argument(
                    "FCFSBatchScheduler::scheduleOnHosts(): We don't support " + host_selection_algorithm +
                    " as host selection algorithm");
        }
    }

    std::map<std::string, double> FCFSBatchScheduler::getStartTimeEstimates(
            std::set<std::tuple<std::string, unsigned long, unsigned long, sg_size_t>> set_of_jobs) {
        if (cs->getPropertyValueAsString(BatchComputeServiceProperty::HOST_SELECTION_ALGORITHM) != "FIRSTFIT") {
            throw std::runtime_error("FCFSBatchScheduler::getStartTimeEstimates(): The fcfs scheduling algorithm can only provide start time estimates "
                                     "when the HOST_SELECTION_ALGORITHM property is set to FIRSTFIT");
        }

        // Assumes time origin is zero for simplicity!

        // Set the available time of each node to zero (i.e., now)
        // (invariant: for each host, core availabilities are sorted by
        //             non-decreasing available time)
        std::map<simgrid::s4u::Host *, std::vector<double>> core_available_times;
        for (auto h: cs->nodes_to_cores_map) {
            auto host = h.first;
            unsigned long num_cores = h.second;
            std::vector<double> zeros;
            for (unsigned int i = 0; i < num_cores; i++) {
                zeros.push_back(0);
            }
            core_available_times[host] = zeros;
        }


        // Update core availabilities for jobs that are currently running
        for (auto const &job: cs->running_jobs) {
            auto batch_job = job.second;
            double time_to_finish = std::max<double>(0, batch_job->getBeginTimestamp() +
                                                                static_cast<double>(batch_job->getRequestedTime()) -
                                                                wrench::Simulation::getCurrentSimulatedDate());
            for (auto resource: batch_job->getResourcesAllocated()) {
                auto host = resource.first;
                unsigned long num_cores = std::get<0>(resource.second);
                // sg_size_t ram = std::get<1>(resource.second);
                // Update available_times
                double new_available_time = *(core_available_times[host].begin() + static_cast<double>(num_cores) - 1) + time_to_finish;
                for (unsigned int i = 0; i < num_cores; i++) {
                    *(core_available_times[host].begin() + i) = new_available_time;
                }
                // Sort them!
                std::sort(core_available_times[host].begin(), core_available_times[host].end());
            }
        }

#if 0
        std::cerr << "TIMELINES AFTER ACCOUNTING FOR RUNNING JOBS: \n";
      for (auto h : core_available_times) {
        std::cerr << "  " << h.first << ":\n";
        for (auto t : h.second) {
          std::cerr << "     core : " << t << "\n";
        }
      }
      std::cerr << "----------------------------------------\n";
#endif

        // Go through the pending jobs and update core availabilities
        for (auto const &job: this->cs->batch_queue) {
            double duration = static_cast<double>(job->getRequestedTime());
            unsigned long num_hosts = job->getRequestedNumNodes();
            unsigned long num_cores_per_host = job->getRequestedCoresPerNode();

#if 0
            std::cerr << "ACCOUNTING FOR RUNNING JOB WITH " << "duration:" <<
                  duration << " num_hosts=" << num_hosts << " num_cores_per_host=" << num_cores_per_host << "\n";
#endif

            // Compute the  earliest start times on all hosts
            std::vector<std::pair<simgrid::s4u::Host *, double>> earliest_start_times;
            for (auto h: core_available_times) {
                double earliest_start_time = *(h.second.begin() + num_cores_per_host - 1);
                earliest_start_times.emplace_back(std::make_pair(h.first, earliest_start_time));
            }

            // Sort the hosts by earliest start times
            std::sort(earliest_start_times.begin(), earliest_start_times.end(),
                      [](std::pair<simgrid::s4u::Host *, double> const &a, std::pair<simgrid::s4u::Host *, double> const &b) {
                          return std::get<1>(a) < std::get<1>(b);
                      });

            // Compute the actual earliest start time
            double earliest_job_start_time = ((earliest_start_times.begin() + num_hosts - 1))->second;

            // Update the core available times on each host used for the job
            for (unsigned int i = 0; i < num_hosts; i++) {
                auto host = ((earliest_start_times.begin() + i))->first;
                for (unsigned int j = 0; j < num_cores_per_host; j++) {
                    *(core_available_times[host].begin() + j) = earliest_job_start_time + duration;
                }
                std::sort(core_available_times[host].begin(), core_available_times[host].end());
            }

            // Go through all hosts and make sure that no core is available before earliest_job_start_time
            // since this is a simple fcfs algorithm with no "jumping ahead" of any kind
            for (auto h: cs->nodes_to_cores_map) {
                auto host = h.first;
                unsigned long num_cores = h.second;
                for (unsigned int i = 0; i < num_cores; i++) {
                    if (*(core_available_times[host].begin() + i) < earliest_job_start_time) {
                        *(core_available_times[host].begin() + i) = earliest_job_start_time;
                    }
                }
                std::sort(core_available_times[host].begin(), core_available_times[host].end());
            }

#if 0
        std::cerr << "AFTER ACCOUNTING FOR THIS JOB: \n";
        for (auto h : core_available_times) {
          std::cerr << "  " << h.first << ":\n";
          for (auto t : h.second) {
            std::cerr << "     core : " << t << "\n";
          }
        }
        std::cerr << "----------------------------------------\n";
#endif
        }

        // We now have the predicted available times for each core given
        // everything that's running and pending. We can compute predictions.
        std::map<std::string, double> predictions;

        for (auto job: set_of_jobs) {
            std::string id = std::get<0>(job);
            unsigned int num_hosts = std::get<1>(job);
            unsigned int num_cores_per_host = std::get<2>(job);
            // double duration = std::get<3>(job);

            double earliest_job_start_time;

            if ((num_hosts > core_available_times.size()) ||
                (num_cores_per_host > cs->num_cores_per_node)) {
                earliest_job_start_time = -1.0;

            } else {
#if 0
                std::cerr << "COMPUTING PREDICTIONS for JOB: num_hosts=" << num_hosts <<
                    ", num_cores_per_hosts=" << num_cores_per_host << ", duration=" << duration << "\n";
#endif

                // Compute the earliest start times on all hosts
                std::vector<std::pair<simgrid::s4u::Host *, double>> earliest_start_times;
                for (auto h: core_available_times) {
                    double earliest_start_time = *(h.second.begin() + num_cores_per_host - 1);
                    earliest_start_times.emplace_back(std::make_pair(h.first, earliest_start_time));
                }


                // Sort the hosts by putative start times
                std::sort(earliest_start_times.begin(), earliest_start_times.end(),
                          [](std::pair<simgrid::s4u::Host *, double> const &a, std::pair<simgrid::s4u::Host *, double> const &b) {
                              return std::get<1>(a) < std::get<1>(b);
                          });

                earliest_job_start_time = ((earliest_start_times.begin() + num_hosts - 1))->second;
            }


            // Note that below we translate predictions back to actual start dates given the current time
            if (earliest_job_start_time > 0) {
                earliest_job_start_time = wrench::Simulation::getCurrentSimulatedDate() + earliest_job_start_time;
            }
            predictions.insert(std::make_pair(id, earliest_job_start_time));
        }

        return predictions;
    }


    /**
     * @brief Method to process queued  jobs
     */
    void FCFSBatchScheduler::processQueuedJobs() {
        while (true) {
            // Invoke the scheduler to pick a job to schedule
            auto batch_job = this->pickNextJobToSchedule();
            if (batch_job == nullptr) {
                WRENCH_INFO("No pending jobs to schedule");
                break;
            }

            // Get the compound job associated to the picked BatchComputeService job
            std::shared_ptr<CompoundJob> compound_job = batch_job->getCompoundJob();

            // Find on which resources to actually run the job
            unsigned long cores_per_node_asked_for = batch_job->getRequestedCoresPerNode();
            unsigned long num_nodes_asked_for = batch_job->getRequestedNumNodes();
            unsigned long requested_time = batch_job->getRequestedTime();

            //      WRENCH_INFO("Trying to see if I can run job (batch_job = %ld)(%s)",
            //                  (unsigned long)batch_job,
            //                  workflow_job->getName().c_str());
            //        std::map<std::string, std::tuple<unsigned long, double>> resources;

            auto resources = this->scheduleOnHosts(num_nodes_asked_for, cores_per_node_asked_for, ComputeService::ALL_RAM);
            if (resources.empty()) {
                //                WRENCH_INFO("Can't run job %s right now", workflow_job->getName().c_str());
                break;
            }

            WRENCH_INFO("Starting compound job %s", compound_job->getName().c_str());

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
     * @brief No-op method
     * @param batch_job: a BatchComputeService job
     */
    void FCFSBatchScheduler::processJobSubmission(std::shared_ptr<BatchJob> batch_job) {
        // Do nothing
    }

    /**
     * @brief No-op method
     * @param batch_job: a BatchComputeService job
     */
    void FCFSBatchScheduler::processJobFailure(std::shared_ptr<BatchJob> batch_job) {
        // Do nothing
    }

    /**
     * @brief No-op method
     * @param batch_job: a BatchComputeService job
     */
    void FCFSBatchScheduler::processJobCompletion(std::shared_ptr<BatchJob> batch_job) {
        // Do nothing
    }

    /**
     * @brief No-op method
     * @param job_id: a BatchComputeService job id
     */
    void processUnknownJobTermination(std::string job_id) {
        // Do nothing
    }

    /**
     * @brief No-op method
     * @param batch_job: a BatchComputeService job
     */
    void FCFSBatchScheduler::processJobTermination(std::shared_ptr<BatchJob> batch_job) {
        // Do nothing
    }


}// namespace wrench
