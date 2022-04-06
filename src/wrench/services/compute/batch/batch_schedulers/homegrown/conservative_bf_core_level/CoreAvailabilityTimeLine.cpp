/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <iostream>
#include <set>
#include <algorithm>
#include "wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf_core_level/CoreAvailabilityTimeLine.h"
#include <boost/icl/interval_map.hpp>
#include <utility>
#include <wrench/services/compute/batch/BatchJob.h>

#include <wrench/logging/TerminalOutput.h>

WRENCH_LOG_CATEGORY(wrench_core_core_availability_time_line, "Log category for CoreAvailabilityTimeLine");


namespace wrench {

    /**
     * @brief Overloaded operator
     * @param left: left-hand side
     * @param right: right-hand side
     * @return
     */
    bool operator==(const BatchJobSetCoreLevel &left, const BatchJobSetCoreLevel &right) {
        return left.jobs == right.jobs;
    }

    /**
     * @brief Constructor
     * @param max_num_nodes: number of nodes on the platform
     * @param max_num_cores_per_node: number of cores per node on the platform
     */
    CoreAvailabilityTimeLine::CoreAvailabilityTimeLine(unsigned long max_num_nodes, unsigned long max_num_cores_per_node) : max_num_nodes(max_num_nodes) {
        std::set<BatchJob *> empty_set;
        this->availability_timeslots += make_pair(boost::icl::interval<u_int32_t>::right_open(0, UINT32_MAX),
                                                  BatchJobSetCoreLevel());
        this->max_num_nodes = max_num_nodes;
        this->max_num_cores_per_node = max_num_cores_per_node;

        for (int i = 0; i < this->max_num_nodes; i++) {
            this->integer_sequence.insert(this->integer_sequence.end(), i);
        }
    }

    /**
     * @brief Method to clear the node availability timeline
     */
    void CoreAvailabilityTimeLine::clear() {
        this->availability_timeslots.clear();
        std::set<BatchJob *> empty_set;
        this->availability_timeslots += make_pair(boost::icl::interval<u_int32_t>::right_open(0, UINT32_MAX),
                                                  BatchJobSetCoreLevel());
    }

    /**
     * @brief Method to set the node availability timeline's time origin
     * @param t: a date
     */
    void CoreAvailabilityTimeLine::setTimeOrigin(u_int32_t t) {

        while (true) {
            auto ts = this->availability_timeslots.begin();
            if (ts->first.lower() >= t) {
                return;
            } else if (ts->first.upper() <= t) {
                this->availability_timeslots.erase(ts);
                continue;
            } else {
                u_int32_t new_left = t;
                u_int32_t new_right = ts->first.upper();
                BatchJobSetCoreLevel job_set = ts->second;
                this->availability_timeslots.erase(ts);
                this->availability_timeslots += make_pair(
                        boost::icl::interval<u_int32_t>::right_open(new_left, new_right), job_set);
                return;
            }
        }
    }

    /**
     * @brief Method to print the node availability timeline
     */
    void CoreAvailabilityTimeLine::print() {
        std::cerr << "------ SCHEDULE -----\n";
        for (auto &availability_timeslot: this->availability_timeslots) {
            std::cerr << availability_timeslot.first << "(";
            for (int i = 0; i < this->max_num_nodes; i++) {
                std::cerr << availability_timeslot.second.core_utilization[i] << " ";
            }
            std::cerr << ") | ";
            for (auto const &j: availability_timeslot.second.jobs) {
                std::cerr << "j=" << j->getJobID() << "(" << j->getRequestedNumNodes() << "/" << j->getRequestedCoresPerNode() << ") ";
            }
            std::cerr << "\n";
        }
        std::cerr << "---- END SCHEDULE ---\n";
    }

    /**
     * @brief Method to update the node availability timeline
     * @param add: true if we're adding, false otherwise
     * @param start: the start date
     * @param end: the end date
     * @param job: the BatchComputeService job
     */
    void CoreAvailabilityTimeLine::update(bool add, u_int32_t start, u_int32_t end, std::shared_ptr<BatchJob> job) {
        auto job_set = new BatchJobSetCoreLevel();
        job_set->add(std::move(job));

        if (add) {
            this->availability_timeslots +=
                    make_pair(boost::icl::interval<u_int32_t>::right_open(start, end), *job_set);
        } else {
            this->availability_timeslots -=
                    make_pair(boost::icl::interval<u_int32_t>::right_open(start, end), *job_set);
        }
    }

    /**
     * @brief Method to find the earliest start time for a job spec
     * @param duration: the job's duration
     * @param num_nodes: the job's number of nodes
     * @param num_cores_per_node: the job's number of cores per node
     * @return a date and a set of host indices
     */
    std::pair<u_int32_t, std::vector<int>> CoreAvailabilityTimeLine::findEarliestStartTime(uint32_t duration, unsigned long num_nodes, unsigned long num_cores_per_node) {


        uint32_t start_time = UINT32_MAX;
        uint32_t remaining_duration = duration;

        //        WRENCH_INFO("IN FIND EARLIEST START TIME for job with %lu nodes and %lu cores per node", num_nodes, num_cores_per_node);
        // Assume all nodes are feasible
        std::set<int> possible_node_indices = this->integer_sequence;

        for (auto &availability_timeslot: this->availability_timeslots) {

            //            std::cerr << "LOOKING AT A TIME SLOT " <<  availability_timeslot.first << "\n";
            //            std::cerr << " RIGHT NOW POSSIBLE NODES: " << possible_node_indices.size() << "\n";
            // Remove infeasible hosts
            for (int i = 0; i < this->max_num_nodes; i++) {
                //                std::cerr << "  - " << availability_timeslot.second.core_utilization[i]  << "\n";
                if (availability_timeslot.second.core_utilization[i] + num_cores_per_node > this->max_num_cores_per_node) {
                    possible_node_indices.erase(i);
                }
            }

            //            std::cerr << " AFTER REMOVAL NOW POSSIBLE NODES: " << possible_node_indices.size() << "\n";

            // Nope!
            if (possible_node_indices.size() < num_nodes) {
                start_time = UINT32_MAX;
                // Assume all nodes are feasible again
                possible_node_indices = this->integer_sequence;
                continue;
            }
            //            WRENCH_INFO("TIME SLOT IS FEASIBLE!");
            u_int32_t interval_length = availability_timeslot.first.upper() - availability_timeslot.first.lower();

            // Yes!
            if (interval_length >= remaining_duration) {
                if (start_time == UINT32_MAX) {
                    start_time = availability_timeslot.first.lower();
                }
                break;
            }

            // Maybe!
            remaining_duration -= interval_length;
            if (start_time == UINT32_MAX) {
                start_time = availability_timeslot.first.lower();
            }
        }

        // Convert to a sorted, truncated, vector
        std::vector<int> node_indices_to_return(possible_node_indices.begin(), possible_node_indices.end());
        std::sort(node_indices_to_return.begin(), node_indices_to_return.end());
        if (node_indices_to_return.size() > num_nodes) {
            node_indices_to_return.resize(num_nodes);
        }

        return std::make_pair(start_time, node_indices_to_return);
    }

    /**
     * @brief Get the BatchComputeService jobs in the first slot in the node availability timeline
     * @return a set of BatchComputeService jobs
     */
    std::set<std::shared_ptr<BatchJob>> CoreAvailabilityTimeLine::getJobsInFirstSlot() {
        std::set<std::shared_ptr<BatchJob>> to_return;
        for (auto const &j: (*(this->availability_timeslots.begin())).second.jobs) {
            to_return.insert(j);
        }
        return to_return;
    }

}// namespace wrench
