/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** An execution controller implementation that generates job specifications and
 ** sends them to batch service controllers
 **/

#include <iostream>

#include "JobGenerationController.h"

WRENCH_LOG_CATEGORY(job_generation_controller, "Log category for JobGenerationController");

namespace wrench {

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param hostname: the name of the host on which to start the controller
     * @param num_jobs: the number of jobs
     * @param batch_service_controllers: the batch compute service controllers
     */
    JobGenerationController::JobGenerationController(const std::string &hostname,
                int num_jobs,
                const std::vector<std::shared_ptr<BatchServiceController>> batch_service_controllers) : ExecutionController(hostname, "me"),
                                                                                        _num_jobs(num_jobs), _bath_service_controllers(batch_service_controllers) {
    }

    /**
     * @brief main method of the JobGenerationController daemon
     *
     * @return 0 on completion
     */
    int JobGenerationController::main() {

        WRENCH_INFO("Job generation controller starting on host %s at time %lf",
                    Simulation::getHostName().c_str(),
                    Simulation::getCurrentSimulatedDate());

        while (true) {
            this->waitForAndProcessNextEvent();
        }
    }

    void JobGenerationController::processEventCustom(const std::shared_ptr<CustomEvent> &event) {


    }

}// namespace wrench
