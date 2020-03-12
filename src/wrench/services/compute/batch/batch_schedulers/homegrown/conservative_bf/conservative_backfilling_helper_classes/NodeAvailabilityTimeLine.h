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
#include "BatchJobSet.h"

/***********************/
/** \cond INTERNAL     */
/***********************/

namespace wrench {


    class BatchJob;

    class NodeAvailabilityTimeLine {

    public:
        explicit NodeAvailabilityTimeLine(unsigned long max_num_nodes);
        void setTimeOrigin(u_int32_t t);
        u_int32_t getTimeOrigin();
        void add(u_int32_t start, u_int32_t end, BatchJob *job) { update(true, start, end, job);}
        void remove(u_int32_t start, u_int32_t end, BatchJob *job) { update(false, start, end, job);}
        void clear();
        void print();
        std::set<BatchJob *> getJobsInFirstSlot();
        u_int32_t findEarliestStartTime(uint32_t duration, unsigned long num_nodes);



    private:

        unsigned long max_num_nodes;
//        boost::icl::interval_map<u_int32_t, std::set<BatchJob *>,  boost::icl::partial_enricher> availability_timeslots;
        boost::icl::interval_map<u_int32_t, BatchJobSet, boost::icl::partial_enricher> availability_timeslots;

        void update(bool add, u_int32_t start, u_int32_t end, BatchJob *job);

        static unsigned  long sumNodes(const std::set<BatchJob *>& job_set);

    };

}

/***********************/
/** \endcond           */
/***********************/

#endif //WRENCH_NODEAVAILABILITYTIMELINE_H
