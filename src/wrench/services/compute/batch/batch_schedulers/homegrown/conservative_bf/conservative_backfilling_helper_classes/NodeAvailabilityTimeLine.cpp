/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <iostream>
#include <set>
#include "NodeAvailabilityTimeLine.h"
#include <boost/icl/interval_map.hpp>
#include <wrench/services/compute/batch/BatchJob.h>

namespace wrench {

    NodeAvailabilityTimeLine::NodeAvailabilityTimeLine(unsigned long max_num_nodes) : max_num_nodes(max_num_nodes) {

        std::set<BatchJob *> empty_set;
        this->availability_timeslots += make_pair(boost::icl::interval<u_int32_t>::right_open(0, UINT32_MAX),
                                                  empty_set);
    }

    void NodeAvailabilityTimeLine::clear() {
        this->availability_timeslots.clear();
        std::set<BatchJob *> empty_set;
        this->availability_timeslots += make_pair(boost::icl::interval<u_int32_t>::right_open(0, UINT32_MAX),
                                                  empty_set);
    }

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
                std::set<BatchJob *> job_set = ts->second;
                this->availability_timeslots.erase(ts);
                this->availability_timeslots += make_pair(
                        boost::icl::interval<u_int32_t>::right_open(new_left, new_right), job_set);
                return;
            }
        }

    }

    void NodeAvailabilityTimeLine::print() {
        std::cerr  << "------ SCHEDULE -----\n";
        for (auto it = this->availability_timeslots.begin(); it != this->availability_timeslots.end(); it++) {
            std::cerr << (*it).first << "| ";
            for (auto const &j  : (*it).second) {
                std::cerr << j->getJobID() << " ";
            }
            std::cerr << "\n";
        }
        std::cerr  << "---- END SCHEDULE ---\n";
    }

    bool NodeAvailabilityTimeLine::canStartNow(BatchJob *job) {
        return (findEarliestStartTimeHelper(true, job->getRequestedTime(), job->getRequestedNumNodes()) != UINT32_MAX);
    }

    u_int32_t NodeAvailabilityTimeLine::findEarliestStartTime(uint32_t duration, unsigned long num_nodes) {
        return findEarliestStartTimeHelper(false, duration, num_nodes);
    }

    void NodeAvailabilityTimeLine::update(bool add, u_int32_t start, u_int32_t end, BatchJob *job) {
        std::set<BatchJob *> job_set = {job};

        if (add) {
            this->availability_timeslots +=
                    make_pair(boost::icl::interval<u_int32_t>::right_open(start, end), job_set);
        } else {
            this->availability_timeslots -=
                    make_pair(boost::icl::interval<u_int32_t>::right_open(start, end), job_set);
        }
    }


    u_int32_t
    NodeAvailabilityTimeLine::findEarliestStartTimeHelper(bool at_origin, uint32_t duration, unsigned long num_nodes) {
        uint32_t start_time = UINT32_MAX;
        uint32_t remaining_duration = duration;


        for (auto it = this->availability_timeslots.begin(); it != this->availability_timeslots.end(); it++) {
            unsigned long available_nodes = this->max_num_nodes - sumNodes((*it).second);
            // Nope!
            if (available_nodes < num_nodes) {
                start_time = UINT32_MAX;
                if (at_origin) break;
                continue;
            }
            u_int32_t interval_length = (*it).first.upper() - (*it).first.lower();
            // Yes!
            if (interval_length >= remaining_duration) {
                if (start_time == UINT32_MAX) {
                    start_time = (*it).first.lower();
                }
                break;
            }
            // Maybe!
            remaining_duration -= interval_length;
            if (start_time == UINT32_MAX) {
                start_time = (*it).first.upper();
            }
        }
        return start_time;
    }

    unsigned long NodeAvailabilityTimeLine::sumNodes(const std::set<BatchJob *>& job_set) {
        unsigned long sum = 0;
        for (auto const  &j : job_set) {
            sum += j->getRequestedNumNodes();
        }
        return sum;
    }

    std::set<BatchJob *> NodeAvailabilityTimeLine::getJobsInFirstSlot() {
        std::set<BatchJob *> to_return;
        for (auto const &j : (*(this->availability_timeslots.begin())).second) {
            to_return.insert(j);
        }

        return to_return;
    }

    u_int32_t NodeAvailabilityTimeLine::getTimeOrigin() {
        return (*(this->availability_timeslots.begin())).first.lower();
    }


}
