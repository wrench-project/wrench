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
#include "WorkloadTraceFileReplayer.h"

namespace wrench {

    /**
     * @brief Constructor
     * @param simulation: a pointer to the simulation object
     * @param hostname: the hostname on which the trace file replayer will run
     * @param batch_service: the batch service to which it submits jobs
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
            batch_service(batch_service)
    {}

    int WorkloadTraceFileReplayer::main() {

      for (auto job : this->workload_trace) {
        // Sleep until the submission time
        double sub_time = std::get<1>(job);
        double curtime = S4U_Simulation::getClock();
        double sleeptime = sub_time - curtime;
        if (sleeptime > 0)
          wrench::S4U_Simulation::sleep(sleeptime);

        // Get job information
        std::string id = std::get<0>(job);
        double flops = std::get<2>(job);
        double requested_flops = std::get<3>(job);
        double requested_ram = std::get<4>(job);
        int num_nodes = std::get<5>(job);

//        // Get the number of cores per node
//        // TODO: This is pretty ugly...
//        unsigned long num_cores = this->batch_service->nodes_to_cores_map.begin()->second;
//
//        // Create a list of tasks the use all resources
//        std::vector<WorkflowTask *> tasks;
//        for (size_t i=0; i < num_nodes; i++) {
//          tasks.push_back(new WorkflowTask());
//        }
//
//
//
//        // Create job that takes up all resources
////          std::cerr << "SUBMITTING " << "sub="<< sub_time << "num_nodes=" << num_nodes << " id="<<id << " flops="<<flops << " rflops="<<requested_flops << " ram="<<requested_ram << "\n";
//        WorkflowTask *task = this->workflow->addTask(id, flops, min_num_cores, max_num_cores,
//                                                             parallel_efficiency);
//
//        wrench::StandardJob *standard_job = job_manager->createStandardJob(
//                {task},
//                {},
//                {},
//                {},
//                {});
//
//
//        std::map<std::string, std::string> batch_job_args;
//        batch_job_args["-N"] = std::to_string(num_nodes);
//        batch_job_args["-t"] = std::to_string(requested_flops);
//        batch_job_args["-c"] = std::to_string(max_num_cores); //use all cores
//        try {
//          job_manager->submitJob(standard_job, this->test->compute_service, batch_job_args);
//        } catch (wrench::WorkflowExecutionException &e) {
//          throw std::runtime_error("Failed to submit a job");
//        }
//
//        num_submitted_jobs++;

//      }
    }

    return 0;
}

};