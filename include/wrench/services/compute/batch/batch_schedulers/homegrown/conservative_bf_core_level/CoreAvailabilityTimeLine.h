/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_COREAVAILABILITYTIMELINE_H
#define WRENCH_COREAVAILABILITYTIMELINE_H

#include <vector>
#include <boost/icl/interval_map.hpp>
#include "wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf_core_level/BatchJobSetCoreLevel.h"

/***********************/
/** \cond              */
/***********************/

namespace wrench {


    class BatchJob;

    /**
     * @brief A class that implements a node availability time line abstraction
     */
    class CoreAvailabilityTimeLine {

    public:
        explicit CoreAvailabilityTimeLine(unsigned long max_num_nodes, unsigned long max_num_cores_per_node);
        void setTimeOrigin(u_int32_t t);
        void add(u_int32_t start, u_int32_t end, std::shared_ptr<BatchJob>job) { update(true, start, end, job);}
        void remove(u_int32_t start, u_int32_t end, std::shared_ptr<BatchJob> job) { update(false, start, end, job);}
        void clear();
        void print();
        std::set<std::shared_ptr<BatchJob>> getJobsInFirstSlot();
        std::pair<u_int32_t, std::vector<int>> findEarliestStartTime(uint32_t duration, unsigned long num_nodes, unsigned long num_cores_per_node);

    private:
        unsigned long max_num_nodes;
        unsigned long max_num_cores_per_node;
        boost::icl::interval_map<u_int32_t, BatchJobSetCoreLevel, boost::icl::partial_enricher> availability_timeslots;

        void update(bool add, u_int32_t start, u_int32_t end, std::shared_ptr<BatchJob>job);

        std::set<int> integer_sequence;

    };

}

/***********************/
/** \endcond           */
/***********************/

#endif //WRENCH_COREEAVAILABILITYTIMELINE_H
