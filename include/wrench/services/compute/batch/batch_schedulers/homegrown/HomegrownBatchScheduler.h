/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_HOMEGROWNBATCHSCHEDULER_H
#define WRENCH_HOMEGROWNBATCHSCHEDULER_H

#include "wrench/services/compute/batch/batch_schedulers/BatchScheduler.h"
#include <stdexcept>


namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief An abstract class that defines a batch scheduler
     */
    class HomegrownBatchScheduler : public BatchScheduler {

    public:
        /**
         * @brief Constructor
         *
         * @param cs: the batch compute service for which this scheduler is operating
         */
        explicit HomegrownBatchScheduler(BatchComputeService *cs) : BatchScheduler(cs) {}

        void init() override{};

        void launch() override{};

        void shutdown() override{};

        void processUnknownJobTermination(std::string job_id) override {
            throw std::runtime_error("HomegrownBatchScheduler::processUnknownJobTermination(): this method should not be called since this scheduler is not Batsched");
        }

        static std::map<simgrid::s4u::Host *, std::tuple<unsigned long, sg_size_t>> selectHostsFirstFit(BatchComputeService *cs,
                                                                                                     unsigned long num_nodes,
                                                                                                     unsigned long cores_per_node,
                                                                                                     sg_size_t ram_per_node);
        static std::map<simgrid::s4u::Host *, std::tuple<unsigned long, sg_size_t>> selectHostsBestFit(BatchComputeService *cs,
                                                                                                    unsigned long num_nodes,
                                                                                                    unsigned long cores_per_node,
                                                                                                    sg_size_t ram_per_node);
        static std::map<simgrid::s4u::Host *, std::tuple<unsigned long, sg_size_t>> selectHostsRoundRobin(BatchComputeService *cs,
                                                                                                       unsigned long *round_robin_host_selector_idx,
                                                                                                       unsigned long num_nodes,
                                                                                                       unsigned long cores_per_node,
                                                                                                       sg_size_t ram_per_node);
        /**
         * @brief Virtual method to figure out on which actual resources a job could be scheduled right now
         * @param num_nodes: number of nodes
         * @param cores_per_node: number of cores per node
         * @param ram_per_node: amount of RAM
         * @return a host:<core,RAM> map
         */
        virtual std::map<simgrid::s4u::Host *, std::tuple<unsigned long, sg_size_t>> scheduleOnHosts(unsigned long num_nodes, unsigned long cores_per_node, sg_size_t ram_per_node) = 0;
    };

    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench


#endif//WRENCH_HOMEGROWNBATCHSCHEDULER_H
