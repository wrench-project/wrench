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

    bool operator == (const BatchJobSet& left, const BatchJobSet& right)
    { return left.jobs==right.jobs; }


    NodeAvailabilityTimeLine::NodeAvailabilityTimeLine(unsigned long max_num_nodes) : max_num_nodes(max_num_nodes) {

        std::set<BatchJob *> empty_set;

//        this->availability_timeslots += make_pair(boost::icl::interval<u_int32_t>::right_open(0, UINT32_MAX), empty_set);
        this->availability_timeslots += make_pair(boost::icl::interval<u_int32_t>::right_open(0, UINT32_MAX), BatchJobSet());
    }

    void NodeAvailabilityTimeLine::clear() {
        this->availability_timeslots.clear();
        std::set<BatchJob *> empty_set;
//        this->availability_timeslots += make_pair(boost::icl::interval<u_int32_t>::right_open(0, UINT32_MAX), empty_set);
        this->availability_timeslots += make_pair(boost::icl::interval<u_int32_t>::right_open(0, UINT32_MAX), BatchJobSet());
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
//                std::set<BatchJob *> job_set = ts->second;
                BatchJobSet job_set = ts->second;
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
//            for (auto const &j  : (*it).second) {
            for (auto const &j  : (*it).second.jobs) {
                std::cerr << j->getJobID() << " ";
            }
            std::cerr << "\n";
        }
        std::cerr  << "---- END SCHEDULE ---\n";
    }



    void NodeAvailabilityTimeLine::update(bool add, u_int32_t start, u_int32_t end, BatchJob *job) {
//        std::set<BatchJob *> job_set = {job};
        auto job_set = new BatchJobSet();
        job_set->add(job);

        if (add) {
//        std::cerr << "IN NodeAvailabilityTimeLine::update(): addind jobset: " << job_set->jobs.size() << "\n";
            this->availability_timeslots +=
                    make_pair(boost::icl::interval<u_int32_t>::right_open(start, end), *job_set);
        } else {
            this->availability_timeslots -=
                    make_pair(boost::icl::interval<u_int32_t>::right_open(start, end), *job_set);
        }
    }

    u_int32_t NodeAvailabilityTimeLine::findEarliestStartTime(uint32_t duration, unsigned long num_nodes) {

        uint32_t start_time = UINT32_MAX;
        uint32_t remaining_duration = duration;


        for (auto it = this->availability_timeslots.begin(); it != this->availability_timeslots.end(); it++) {
//            unsigned long available_nodes = this->max_num_nodes - sumNodes((*it).second);
            unsigned long available_nodes = this->max_num_nodes  - (*it).second.num_nodes_utilized;

//            std::cerr << "*** LOOKING AT SLOT " << (*it).first.lower()  <<  "-"  << (*it).first.upper() << " : "  << available_nodes << " nodes available (I need " +  std::to_string(num_nodes) +  " nodes)\n";
//            std::cerr << "     (start_time  = " << start_time << ")\n";
            // Nope!
            if (available_nodes < num_nodes) {
//                std::cerr << "NOPE\n";
                start_time = UINT32_MAX;
                continue;
            }
            u_int32_t interval_length = (*it).first.upper() - (*it).first.lower();
            // Yes!
            if (interval_length >= remaining_duration) {
//                std::cerr << "YES\n";
                if (start_time == UINT32_MAX) {
                    start_time = (*it).first.lower();
//                    std::cerr << "  SETTING   (start_time  = " << start_time << ")\n";

                }
                break;
            }
            // Maybe!
//            std::cerr << "MAYBE\n";
            remaining_duration -= interval_length;
            if (start_time == UINT32_MAX) {
                start_time = (*it).first.lower();
            }
        }
//        std::cerr << "     (start_time  = " << start_time << ")\n";
        return start_time;
    }

    unsigned long NodeAvailabilityTimeLine::sumNodes(const std::set<BatchJob *>& job_set) {
        unsigned long sum1 = 0, sum2 = 0;
        for (auto const  &j : job_set) {
            sum1 += j->getRequestedNumNodes();
        }
        for (auto const  &j : job_set) {
            sum2 += j->getRequestedNumNodes();
        }
        return std::max(sum1, sum2);
        return  sum1;
    }

    std::set<BatchJob *> NodeAvailabilityTimeLine::getJobsInFirstSlot() {
        std::set<BatchJob *> to_return;
        for (auto const &j : (*(this->availability_timeslots.begin())).second.jobs) {
            to_return.insert(j);
        }

        return to_return;
    }

    u_int32_t NodeAvailabilityTimeLine::getTimeOrigin() {
        return (*(this->availability_timeslots.begin())).first.lower();
    }


}
