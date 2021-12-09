/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** A Controller that creates several multi-action jobs and runs them
 **/

#include <iostream>

#include "MultiActionMultiJobController.h"

WRENCH_LOG_CATEGORY(custom_controlle, "Log category for MultiActionMultiJobController");

namespace wrench {

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param compute_services: a set of compute services available to run tasks
     * @param storage_services: a set of storage services available to store files
     * @param hostname: the name of the host on which to start the Controller
     */
    MultiActionMultiJobController::MultiActionMultiJobController(
                                 std::shared_ptr<BareMetalComputeService> bm_cs,
                                 std::shared_ptr<CloudComputeService> cloud_cs,
                                 std::shared_ptr<StorageService> ss,
                                 const std::string &hostname) : ExecutionController(
            hostname,
            "mamj") {}

    /**
     * @brief main method of the MultiActionMultiJobController daemon
     *
     * @return 0 on completion
     *
     * @throw std::runtime_error
     */
    int MultiActionMultiJobController::main() {

        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        WRENCH_INFO("Controller starting on host %s", Simulation::getHostName().c_str());

        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();


        WRENCH_INFO("Controller terminating");
        return 0;
    }

}
