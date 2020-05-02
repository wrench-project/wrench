/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "HadoopMRJobWMS.h"

#include <wrench/services/compute/hadoop/HadoopComputeService.h>
#include <wrench/services/compute/hadoop/DeterministicMRJob.h>

#define GB 1000000000.0

XBT_LOG_NEW_DEFAULT_CATEGORY(custom_wms, "Log category for HadoopMRJobWMS");

namespace wrench {

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param compute_services: a set of compute services available to run tasks
     * @param storage_services: a set of storage services available to store files
     * @param hostname: the name of the host on which to start the WMS
     */
    HadoopMRJobWMS::HadoopMRJobWMS(const std::string &hostname) : WMS(
            nullptr, nullptr,
            {},
            {},
            {}, nullptr,
            hostname,
            "two-tasks-at-a-time-virtualized-cluster") {
    }

    /**
     * @brief main method of the HadoopMRJobWMS daemon
     *
     * @return 0 on completion
     *
     * @throw std::runtime_error
     */
    int HadoopMRJobWMS::main() {
        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        WRENCH_INFO("WMS starting on host %s", Simulation::getHostName().c_str());

        /* Instantiate a HadoopService */
        WRENCH_INFO("Starting a Hadoop Service...");
        std::set<std::string> hadoop_hosts = {"VirtualizedClusterHost1", "VirtualizedClusterHost2"};
        auto hadoop_service = std::shared_ptr<HadoopComputeService>(new wrench::HadoopComputeService(
                "VirtualizedClusterProviderHost", hadoop_hosts, {}, {}));
        hadoop_service->simulation = this->simulation;
        hadoop_service->start(hadoop_service, true, false);

        WRENCH_INFO("Creating a MR Job");
        /**
        DeterministicMRJob::DeterministicMRJob(int num_reducers, double data_size, int block_size,
                                               bool use_combiner, int sort_factor, double spill_percent,
                                               int mapper_key_width, int mapper_value_width, std::vector<int> &files,
                                               double mapper_flops, int reducer_key_width, int reducer_value_width,
                                               double reducer_flops)
         */
        std::vector<int> files = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100};
        auto mr_job = new DeterministicMRJob(1, 1000.0, 16,
                false, 1, 0.8, 16,
                16, files, 1.0, 1,
                1, 1.0);

        WRENCH_INFO("Submitting  the MR Job to the Hadoop Service!");
        hadoop_service->runMRJob(mr_job);

        WRENCH_INFO("Waiting for next event");
        // Not sure what's up, this is causing an "Oops Deadlock, or code not perfectly clean" error.
        // auto event = this->waitForNextEvent();

        WRENCH_INFO("Exiting");
        return 0;
    }


}
