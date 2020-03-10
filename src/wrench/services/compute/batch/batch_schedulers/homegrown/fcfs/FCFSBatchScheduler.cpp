/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "FCFSBatchScheduler.h"
#include "wrench/simulation/Simulation.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/compute/batch/BatchComputeService.h"

WRENCH_LOG_NEW_DEFAULT_CATEGORY(fcfs_batch_scheduler, "Log category for FCFSBatchScheduler");

namespace wrench {

    /**
    * @brief Overriden Method to pick the next job to schedule
    * @return A batch job, or nullptr is none is found
    */
    BatchJob *FCFSBatchScheduler::pickNextJobToSchedule() {
        if (this->cs->batch_queue.empty()) {
            return nullptr;
        } else {
            return *this->cs->batch_queue.begin();
        }
    }


    /**
     * #brief Override Method to find hosts on which to scheduled a  job
     * @param num_nodes: the job's requested num nodes
     * @param cores_per_node: the job's num cores per node
     * @param ram_per_node: the job's ram per node
     * @return A resource list
     */
    std::map<std::string, std::tuple<unsigned long, double>> FCFSBatchScheduler::scheduleOnHosts(
            unsigned long num_nodes, unsigned long cores_per_node, double ram_per_node) {

        if (ram_per_node == ComputeService::ALL_RAM) {
            ram_per_node = Simulation::getHostMemoryCapacity(cs->available_nodes_to_cores.begin()->first);
        }
        if (cores_per_node == ComputeService::ALL_CORES) {
            cores_per_node = Simulation::getHostNumCores(cs->available_nodes_to_cores.begin()->first);
        }

        if (ram_per_node > Simulation::getHostMemoryCapacity(cs->available_nodes_to_cores.begin()->first)) {
            throw std::runtime_error("FCFSBatchScheduler::findNextJobToSchedule(): Asking for too much RAM per host");
        }
        if (num_nodes > cs->available_nodes_to_cores.size()) {
            throw std::runtime_error("FCFSBatchScheduler::findNextJobToSchedule(): Asking for too many hosts");
        }
        if (cores_per_node > Simulation::getHostNumCores(cs->available_nodes_to_cores.begin()->first)) {
            throw std::runtime_error("FCFSBatchScheduler::findNextJobToSchedule(): Asking for too many cores per host");
        }

        std::map<std::string, std::tuple<unsigned long, double>> resources = {};
        std::vector<std::string> hosts_assigned = {};
        auto host_selection_algorithm = this->cs->getPropertyValueAsString(BatchComputeServiceProperty::HOST_SELECTION_ALGORITHM);

        if (host_selection_algorithm == "FIRSTFIT") {
            std::map<std::string, unsigned long>::iterator map_it;
            unsigned long host_count = 0;
            for (map_it = cs->available_nodes_to_cores.begin();
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
                for (vector_it = hosts_assigned.begin(); vector_it != hosts_assigned.end(); vector_it++) {
                    cs->available_nodes_to_cores[*vector_it] += cores_per_node;
                }
            }
        } else if (host_selection_algorithm == "BESTFIT") {
            while (resources.size() < num_nodes) {
                unsigned long target_slack = 0;
                std::string target_host = "";
                unsigned long target_num_cores = 0;

                for (auto h : cs->available_nodes_to_cores) {
                    std::string hostname = std::get<0>(h);
                    unsigned long num_available_cores = std::get<1>(h);
                    if (num_available_cores < cores_per_node) {
                        continue;
                    }
                    unsigned long tentative_target_num_cores = std::min(num_available_cores, cores_per_node);
                    unsigned long tentative_target_slack =
                            num_available_cores - tentative_target_num_cores;

                    if (target_host.empty() ||
                        (tentative_target_num_cores > target_num_cores) ||
                        ((tentative_target_num_cores == target_num_cores) &&
                         (target_slack > tentative_target_slack))) {
                        target_host = hostname;
                        target_num_cores = tentative_target_num_cores;
                        target_slack = tentative_target_slack;
                    }
                }
                if (target_host.empty()) {
                    WRENCH_INFO("Didn't find a suitable host");
                    resources = {};
                    std::vector<std::string>::iterator it;
                    for (it = hosts_assigned.begin(); it != hosts_assigned.end(); it++) {
                        cs->available_nodes_to_cores[*it] += cores_per_node;
                    }
                    break;
                }
                cs->available_nodes_to_cores[target_host] -= cores_per_node;
                hosts_assigned.push_back(target_host);
                resources.insert(std::make_pair(target_host, std::make_tuple(cores_per_node, ComputeService::ALL_RAM)));
            }
        } else if (host_selection_algorithm == "ROUNDROBIN") {
            static unsigned long round_robin_host_selector_idx = 0;
            unsigned long cur_host_idx = round_robin_host_selector_idx;
            unsigned long host_count = 0;
            do {
                cur_host_idx = (cur_host_idx + 1) % cs->available_nodes_to_cores.size();
                auto it = cs->compute_hosts.begin();
                it = it + cur_host_idx;
                std::string cur_host_name = *it;
                unsigned long num_available_cores = cs->available_nodes_to_cores[cur_host_name];
                if (num_available_cores >= cores_per_node) {
                    cs->available_nodes_to_cores[cur_host_name] -= cores_per_node;
                    hosts_assigned.push_back(cur_host_name);
                    resources.insert(std::make_pair(cur_host_name, std::make_tuple(cores_per_node, ram_per_node)));
                    if (++host_count >= num_nodes) {
                        break;
                    }
                }
            } while (cur_host_idx != round_robin_host_selector_idx);
            if (resources.size() < num_nodes) {
                resources = {};
                std::vector<std::string>::iterator it;
                for (it = hosts_assigned.begin(); it != hosts_assigned.end(); it++) {
                    cs->available_nodes_to_cores[*it] += cores_per_node;
                }
            } else {
                round_robin_host_selector_idx = cur_host_idx;
            }
        } else {
            throw std::invalid_argument(
                    "FCFSBatchScheduler::findNextJobToSchedule(): We don't support " + host_selection_algorithm +
                    " as host selection algorithm"
            );
        }

        return resources;
    }

    std::map<std::string, double> FCFSBatchScheduler::getStartTimeEstimates(
            std::set<std::tuple<std::string, unsigned int, unsigned int, double>> set_of_jobs) {

        if (cs->getPropertyValueAsString(BatchComputeServiceProperty::HOST_SELECTION_ALGORITHM) != "FIRSTFIT") {
            throw std::runtime_error("FCFSBatchScheduler::getStartTimeEstimates(): The fcfs sceduling algorithm can only provide start time estimates "
                                     "when the HOST_SELECTION_ALGORITHM property is set to FIRSTFIT");
        }

        // Assumes time origin is zero for simplicity!

        // Set the available time of each node to zero (i.e., now)
        // (invariant: for each host, core availabilities are sorted by
        //             non-decreasing available time)
        std::map<std::string, std::vector<double>> core_available_times;
        for (auto h : cs->nodes_to_cores_map) {
            std::string hostname = h.first;
            unsigned long num_cores = h.second;
            std::vector<double> zeros;
            for (unsigned int i = 0; i < num_cores; i++) {
                zeros.push_back(0);
            }
            core_available_times.insert(std::make_pair(hostname, zeros));
        }


        // Update core availabilities for jobs that are currently running
        for (auto job : cs->running_jobs) {
            double time_to_finish = std::max<double>(0, job->getBeginTimeStamp() +
                                                        job->getRequestedTime() -
                                                        cs->simulation->getCurrentSimulatedDate());
            for (auto resource : job->getResourcesAllocated()) {
                std::string hostname = resource.first;
                unsigned long num_cores = std::get<0>(resource.second);
                double ram = std::get<1>(resource.second);
                // Update available_times
                double new_available_time = *(core_available_times[hostname].begin() + num_cores - 1) + time_to_finish;
                for (unsigned int i = 0; i < num_cores; i++) {
                    *(core_available_times[hostname].begin() + i) = new_available_time;
                }
                // Sort them!
                std::sort(core_available_times[hostname].begin(), core_available_times[hostname].end());
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
        for (auto job : this->cs->batch_queue) {
            double duration = job->getRequestedTime();
            unsigned long num_hosts = job->getRequestedNumNodes();
            unsigned long num_cores_per_host = job->getRequestedCoresPerNode();

#if 0
            std::cerr << "ACCOUNTING FOR RUNNING JOB WITH " << "duration:" <<
                  duration << " num_hosts=" << num_hosts << " num_cores_per_host=" << num_cores_per_host << "\n";
#endif

            // Compute the  earliest start times on all hosts
            std::vector<std::pair<std::string, double>> earliest_start_times;
            for (auto h : core_available_times) {
                double earliest_start_time = *(h.second.begin() + num_cores_per_host - 1);
                earliest_start_times.emplace_back(std::make_pair(h.first, earliest_start_time));
            }

            // Sort the hosts by earliest start times
            std::sort(earliest_start_times.begin(), earliest_start_times.end(),
                      [](std::pair<std::string, double> const &a, std::pair<std::string, double> const &b) {
                          return std::get<1>(a) < std::get<1>(b);
                      });

            // Compute the actual earliest start time
            double earliest_job_start_time = (*(earliest_start_times.begin() + num_hosts - 1)).second;

            // Update the core available times on each host used for the job
            for (unsigned int i = 0; i < num_hosts; i++) {
                std::string hostname = (*(earliest_start_times.begin() + i)).first;
                for (unsigned int j = 0; j < num_cores_per_host; j++) {
                    *(core_available_times[hostname].begin() + j) = earliest_job_start_time + duration;
                }
                std::sort(core_available_times[hostname].begin(), core_available_times[hostname].end());
            }

            // Go through all hosts and make sure that no core is available before earliest_job_start_time
            // since this is a simple fcfs algorithm with no "jumping ahead" of any kind
            for (auto h : cs->nodes_to_cores_map) {
                std::string hostname = h.first;
                unsigned long num_cores = h.second;
                for (unsigned int i = 0; i < num_cores; i++) {
                    if (*(core_available_times[hostname].begin() + i) < earliest_job_start_time) {
                        *(core_available_times[hostname].begin() + i) = earliest_job_start_time;
                    }
                }
                std::sort(core_available_times[hostname].begin(), core_available_times[hostname].end());
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

        // We now have the predicted available times for each cores given
        // everything that's running and pending. We can compute predictions.
        std::map<std::string, double> predictions;

        for (auto job : set_of_jobs) {
            std::string id = std::get<0>(job);
            unsigned int num_hosts = std::get<1>(job);
            unsigned int num_cores_per_host = std::get<2>(job);
            double duration = std::get<3>(job);

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
                std::vector<std::pair<std::string, double>> earliest_start_times;
                for (auto h : core_available_times) {
                    double earliest_start_time = *(h.second.begin() + num_cores_per_host - 1);
                    earliest_start_times.emplace_back(std::make_pair(h.first, earliest_start_time));
                }


                // Sort the hosts by putative start times
                std::sort(earliest_start_times.begin(), earliest_start_times.end(),
                          [](std::pair<std::string, double> const &a, std::pair<std::string, double> const &b) {
                              return std::get<1>(a) < std::get<1>(b);
                          });

                earliest_job_start_time = (*(earliest_start_times.begin() + num_hosts - 1)).second;
            }


            // Note that below we translate predictions back to actual start dates given the current time
            if (earliest_job_start_time > 0) {
                earliest_job_start_time = cs->simulation->getCurrentSimulatedDate() + earliest_job_start_time;
            }
            predictions.insert(std::make_pair(id, earliest_job_start_time));

        }

        return predictions;
    }

    void FCFSBatchScheduler::processQueuedJobs() {

        while (true) {

            // Invoke the scheduler to pick a job to schedule
            BatchJob *batch_job = this->pickNextJobToSchedule();
            if (batch_job == nullptr) {
                WRENCH_INFO("No pending jobs to schedule");
                break;
            }

            // Get the workflow job associated to the picked batch job
            WorkflowJob *workflow_job = batch_job->getWorkflowJob();

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
                WRENCH_INFO("Can't run job %s right now", workflow_job->getName().c_str());
                break;
            }

            WRENCH_INFO("Starting job %s", workflow_job->getName().c_str());

            // Remove the job from the batch queue
            this->cs->removeJobFromBatchQueue(batch_job);

            // Add it to the running list
            this->cs->running_jobs.insert(batch_job);

            // Start it!
            this->cs->startJob(resources, workflow_job, batch_job, num_nodes_asked_for, requested_time,
                     cores_per_node_asked_for);


        }
    }


    void FCFSBatchScheduler::processJobFailure(BatchJob *batch_job, std::string job_id) {
        // Do nothing
    }

    void FCFSBatchScheduler::processJobCompletion(BatchJob *batch_job, std::string job_id) {
        // Do nothing
    }

    void FCFSBatchScheduler::processJobTermination(BatchJob *batch_job, std::string job_id) {
        // Do nothing
    }


}