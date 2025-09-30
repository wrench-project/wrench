/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** An batch service controller implementation that submits jobs to a batch compute
 ** service or delegate jobs to a peer...
 **/

#include <iostream>

#include "BatchServiceController.h"

WRENCH_LOG_CATEGORY(batch_service_controller, "Log category for BatchServiceController");

namespace wrench {

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param hostname: the name of the host on which to start the controller
     * @param batch_compute_service: the batch compute service this controller is in charge of
     */
    BatchServiceController::BatchServiceController(const std::string &hostname,
                                               const std::shared_ptr<BatchComputeService>& batch_compute_service) : ExecutionController(hostname, "me"),
                                                                                        _batch_compute_service(batch_compute_service) {
    }

    /**
     * @brief main method of the GreedyExecutionController daemon
     *
     * @return 0 on completion
     */
    int BatchServiceController::main() {

        WRENCH_INFO("Batch service execution controller starting on host %s at time %lf",
                    Simulation::getHostName().c_str(),
                    Simulation::getCurrentSimulatedDate());

        while (true) {
            this->waitForAndProcessNextEvent();
        }
    }

    void BatchServiceController::processEventCustom(const std::shared_ptr<CustomEvent> &event) {


    }


    void BatchServiceController::processEventCompoundJobCompletion(const std::shared_ptr<CompoundJobCompletedEvent> &event) {

    }

}// namespace wrench
