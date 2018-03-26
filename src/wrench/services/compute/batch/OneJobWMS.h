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

    class OneJobWMS : public WMS {

    public:
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
