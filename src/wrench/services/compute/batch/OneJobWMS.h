/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_ONEJOBWMS_H
#define WRENCH_ONEJOBWMS_H

#include "wrench/wms/WMS.h"

/***********************/
/** \cond INTERNAL    **/
/***********************/

namespace wrench {

    class BatchService;

    /**
     * @brief A WMS that only submits a single job to a batch service
     */
    class OneJobWMS : public WMS {

    public:
        /**
         * @brief Constructor
         * @param hostname: the name of the host on which the WMS will run
         * @param job_id: the id of the one job it will be executing
         * @param time: the actual time that the job takes
         * @param requested_time: the time to request from the batch service
         * @param requested_ram: the RAM to request from the batch service
         * @param num_nodes: the number of nodes to request from the batch scheduler
         * @param num_cores_per_task: the number of cores per task to be used (on each node)
         * @param batch_service: the bath service
         * @param batch_service_core_flop_rate: the core flop rate of the machines
         *            that the batch service provides access to
         */
        OneJobWMS(std::string hostname, std::string job_id, double time, double requested_time,
                  double requested_ram, int num_nodes,
                  unsigned long num_cores_per_task, ComputeService *batch_service, double batch_service_core_flop_rate) : WMS(nullptr, nullptr,
                                                                               {batch_service}, {},
                                                                               {},
                                                                               nullptr, hostname,
                                                                               "one_job_wms_" +
                                                                               job_id),
                                                                           job_id(job_id),
                                                                           time(time),
                                                                           requested_time(
                                                                                   requested_time),
                                                                           requested_ram(
                                                                                   requested_ram),
                                                                           num_nodes(num_nodes),
                                                                           num_cores_per_task(
                                                                                   num_cores_per_task),
        batch_service_core_flop_rate(batch_service_core_flop_rate) {}

        int main() override;

    private:
        std::string hostname;
        std::string job_id;
        double time;
        double requested_time;
        double requested_ram;
        int num_nodes;
        unsigned long num_cores_per_task;
        double batch_service_core_flop_rate;

    };

};

/***********************/
/** \endcond          **/
/***********************/

#endif //WRENCH_ONEJOBWMS_H
