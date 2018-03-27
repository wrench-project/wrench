/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/simgrid_S4U_util/S4U_Simulation.h>
#include <wrench/services/compute/batch/BatchService.h>
#include <wrench-dev.h>
#include "WorkloadTraceFileReplayer.h"
#include "OneJobWMS.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(workload_trace_file_replayer, "Log category for Trace File Replayer");

namespace wrench {

    /**
     * @brief Constructor
     * @param simulation: a pointer to the simulation object
     * @param hostname: the hostname on which the trace file replayer will run
     * @param batch_service: the batch service to which it submits jobs
     * @param num_cores_per_node: the number of cores per host on the batch-scheduled platform
     * @param workload_trace: the workload trace it replays
     */
    WorkloadTraceFileReplayer::WorkloadTraceFileReplayer(Simulation *simulation,
                                                         std::string hostname,
                                                         BatchService *batch_service,
                                                         unsigned long num_cores_per_node,
                                                         std::vector<std::tuple<std::string, double, double, double, double, unsigned int>> &workload_trace
    ) :
            Service(std::move(hostname), "workload_trace_file_replayer", "workload_trace_file_replayer"),
            workload_trace(workload_trace),
            batch_service(batch_service),
            num_cores_per_node(num_cores_per_node) {}

    int WorkloadTraceFileReplayer::main() {

      WRENCH_INFO("Workload trace file replayer starting!");

      double core_flop_rate = *(this->batch_service->getCoreFlopRate().begin());

      for (auto job : this->workload_trace) {
        // Sleep until the submission time
        double sub_time = std::get<1>(job);
        double curtime = S4U_Simulation::getClock();
        double sleeptime = sub_time - curtime;
        if (sleeptime > 0)
          wrench::S4U_Simulation::sleep(sleeptime);

        // Get job information
        std::string job_id = std::get<0>(job);
        double time = std::get<2>(job);
        double requested_time = std::get<3>(job);
        double requested_ram = std::get<4>(job);
        int num_nodes = std::get<5>(job);

        // Create the OneJobWMS
        std::shared_ptr<OneJobWMS> one_job_wms = std::shared_ptr<OneJobWMS>(
                new OneJobWMS(this->hostname, job_id, time, requested_time, requested_ram, num_nodes,
                              this->num_cores_per_node,
                              this->batch_service,
                core_flop_rate));

        // Start the OneJobWMS
        one_job_wms->simulation = this->simulation;
        one_job_wms->start(one_job_wms, true); // Daemonize!
        // will not get out of scope, but it's ok because of the cool life-saver!

      }

      return 0;
    }

};