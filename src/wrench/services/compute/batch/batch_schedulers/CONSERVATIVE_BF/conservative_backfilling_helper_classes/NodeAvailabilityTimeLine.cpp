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

NodeAvailabilityTimeLine::NodeAvailabilityTimeLine(u_int16_t max_num_nodes) : max_num_nodes(max_num_nodes) {

    this->availability_timeslots.add(make_pair(boost::icl::interval<u_int32_t>::right_open(0, UINT32_MAX), max_num_nodes));

}

void NodeAvailabilityTimeLine::resetTimeOrigin(u_int32_t t) {

    while (true) {
        auto ts = this->availability_timeslots.begin();
        if (ts->first.lower() >= t) {
            return;
        } else if  (ts->first.upper() <=  t) {
            this->availability_timeslots.erase(ts);
            continue;
        } else {
            u_int32_t new_left = t;
            u_int32_t new_right = ts->first.upper();
            u_int16_t new_num_nodes = ts->second;
            this->availability_timeslots.erase(ts);
            this->availability_timeslots.add(make_pair(boost::icl::interval<u_int32_t>::right_open(new_left, new_right), new_num_nodes));
            return;
        }
    }

}

void NodeAvailabilityTimeLine::print() {
    for (auto it = this->availability_timeslots.begin(); it != this->availability_timeslots.end(); it++)  {
        std::cerr << (*it).first  << "|" << (*it).second << "\n";
    }
}

bool NodeAvailabilityTimeLine::canStartAtOrigin(uint32_t duration, u_int16_t num_nodes) {
    return (findEarliestStartTimeHelper(true, duration, num_nodes) != UINT32_MAX);
}

u_int32_t NodeAvailabilityTimeLine::findEarliestStartTime(uint32_t duration, u_int16_t num_nodes) {
    return findEarliestStartTimeHelper(false, duration, num_nodes);
}

void NodeAvailabilityTimeLine::update(bool add, u_int32_t start, u_int32_t end, u_int16_t num_nodes) {
    this->availability_timeslots.add(make_pair(boost::icl::interval<u_int32_t>::right_open(start, end), (add ? num_nodes : -num_nodes)));
}

u_int32_t NodeAvailabilityTimeLine::findEarliestStartTimeHelper(bool at_origin,  uint32_t duration, u_int16_t num_nodes) {
    uint32_t start_time = UINT32_MAX;
    uint32_t remaining_duration = duration;


    for (auto it = this->availability_timeslots.begin(); it != this->availability_timeslots.end(); it++)  {
        // Nope!
        if ((*it).second < num_nodes) {
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



