/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_WORKLOADTRACEFILEREPLAYER_H
#define WRENCH_WORKLOADTRACEFILEREPLAYER_H

#include "wrench/services/Service.h"

/***********************/
/** \cond INTERNAL    **/
/***********************/

namespace wrench {

    class BatchComputeService;

    /**
     * @brief A service that goes through a job submission trace (as loaded
     * by a TraceFileLoader), and "replays" it on a given BatchComputeService.
     */
    class WorkloadTraceFileReplayer : public WMS {

    public:
        WorkloadTraceFileReplayer(std::string hostname,
                                  std::shared_ptr<BatchComputeService> batch_service,
                                  unsigned long num_cores_per_node,
                                  bool use_actual_runtimes_as_requested_runtimes,
                                  std::vector<std::tuple<std::string, double, double, double, double, unsigned int>> &workload_trace
        );

    private:
        std::vector<std::tuple<std::string, double, double, double, double, unsigned int>> &workload_trace;
        std::shared_ptr<BatchComputeService> batch_service;
        unsigned long num_cores_per_node;
        bool use_actual_runtimes_as_requested_runtimes;

        int main() override;
    };

};

/***********************/
/** \endcond          **/
/***********************/



#endif //WRENCH_WORKLOADTRACEFILEREPLAYER_H
