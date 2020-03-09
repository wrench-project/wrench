/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_NODEAVAILABILITYTIMELINE_H
#define WRENCH_NODEAVAILABILITYTIMELINE_H

#include <vector>
#include <boost/icl/interval_map.hpp>

/***********************/
/** \cond INTERNAL     */
/***********************/

class NodeAvailabilityTimeLine {

public:
    explicit NodeAvailabilityTimeLine(u_int16_t max_num_nodes);
    void resetTimeOrigin(u_int32_t t);
    void add(u_int32_t start, u_int32_t end, u_int16_t num_nodes) { update(true, start, end, num_nodes);}
    void remove(u_int32_t start, u_int32_t end, u_int16_t num_nodes) { update(false, start, end, num_nodes);}
    void print();
    bool canStartAtOrigin(uint32_t duration, u_int16_t num_nodes);
    u_int32_t findEarliestStartTime(uint32_t duration, u_int16_t num_nodes);



private:

    unsigned int max_num_nodes;
    boost::icl::interval_map<u_int32_t, u_int16_t,  boost::icl::partial_enricher> availability_timeslots;

    void update(bool add, u_int32_t start, u_int32_t end, u_int16_t num_nodes);
    u_int32_t findEarliestStartTimeHelper(bool at_origin, uint32_t duration, u_int16_t num_nodes);

};


/***********************/
/** \endcond           */
/***********************/

#endif //WRENCH_NODEAVAILABILITYTIMELINE_H
