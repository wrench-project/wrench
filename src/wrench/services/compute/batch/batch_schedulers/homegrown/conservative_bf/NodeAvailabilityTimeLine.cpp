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
#include "wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf/NodeAvailabilityTimeLine.h"
#include <boost/icl/interval_map.hpp>
#include <wrench/services/compute/batch/BatchJob.h>

namespace wrench {

    /**
     * @brief Overloaded operator
     * @param left: left-hand side
     * @param right: right-hand side
     * @return
     */
    bool operator==(const BatchJobSet &left, const BatchJobSet &right) {
        return left.jobs == right.jobs;
    }

    /**
     * @brief Constructor
     * @param max_num_nodes: number of nodes on the platform
     */
    NodeAvailabilityTimeLine::NodeAvailabilityTimeLine(unsigned long max_num_nodes) : max_num_nodes(max_num_nodes) {
        std::set<BatchJob *> empty_set;
        this->availability_timeslots += make_pair(boost::icl::interval<u_int32_t>::right_open(0, UINT32_MAX),
                                                  BatchJobSet());
    }

    /**
     * @brief Method to clear the node availability timeline
     */
    void NodeAvailabilityTimeLine::clear() {
        this->availability_timeslots.clear();
        std::set<BatchJob *> empty_set;
        this->availability_timeslots += make_pair(boost::icl::interval<u_int32_t>::right_open(0, UINT32_MAX),
                                                  BatchJobSet());
    }

    /**
     * @brief Method to set the node availability timeline's time origin
     * @param t: a date
     */
    void NodeAvailabilityTimeLine::setTimeOrigin(u_int32_t t) {

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
                BatchJobSet job_set = ts->second;
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
    void NodeAvailabilityTimeLine::print() {
        std::cerr << "------ SCHEDULE -----\n";
        for (auto &availability_timeslot: this->availability_timeslots) {
            std::cerr << availability_timeslot.first << "(" << availability_timeslot.second.num_nodes_utilized << ") | ";
            for (auto const &j: availability_timeslot.second.jobs) {
                std::cerr << j->getJobID() << "(" << j->getRequestedNumNodes() << ") ";
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
    void NodeAvailabilityTimeLine::update(bool add, u_int32_t start, u_int32_t end, std::shared_ptr<BatchJob> job) {
        auto job_set = new BatchJobSet();
        job_set->add(job);

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
     * @return a date
     */
    u_int32_t NodeAvailabilityTimeLine::findEarliestStartTime(uint32_t duration, unsigned long num_nodes) {


        uint32_t start_time = UINT32_MAX;
        uint32_t remaining_duration = duration;

        for (auto &availability_timeslot: this->availability_timeslots) {
            unsigned long available_nodes = this->max_num_nodes - availability_timeslot.second.num_nodes_utilized;

            // Nope!
            if (available_nodes < num_nodes) {
                start_time = UINT32_MAX;
                continue;
            }
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
        return start_time;
    }

    /**
     * @brief Get the BatchComputeService jobs in the first slot in the node availability timeline
     * @return a set of BatchComputeService jobs
     */
    std::set<std::shared_ptr<BatchJob>> NodeAvailabilityTimeLine::getJobsInFirstSlot() {
        std::set<std::shared_ptr<BatchJob>> to_return;
        for (auto const &j: (*(this->availability_timeslots.begin())).second.jobs) {
            to_return.insert(j);
        }
        return to_return;
    }

}// namespace wrench
