/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_EXAMPLE_BATCH_SERVICE_CONTROLLER_H
#define WRENCH_EXAMPLE_BATCH_SERVICE_CONTROLLER_H

#include <wrench-dev.h>


namespace wrench {

    class Simulation;

    /**
     *  @brief An execution controller implementation
     */
    class BatchServiceController : public ExecutionController {

    public:
        // Constructor
        BatchServiceController(
                const std::string &hostname,
                const std::shared_ptr<BatchComputeService>& batch_compute_service);

        void set_peer(std::shared_ptr<BatchServiceController> peer) { this->_peer = peer; }

    private:
        int main() override;

        const std::shared_ptr<BatchComputeService> _batch_compute_service;
        std::shared_ptr<BatchServiceController> _peer;

        void processEventCustom(const std::shared_ptr<CustomEvent> &event) override;
        void processEventCompoundJobCompletion(const std::shared_ptr<CompoundJobCompletedEvent> &event) override;


    };

}// namespace wrench
#endif//WRENCH_EXAMPLE_JOB_GENERATION_CONTROLLER_H
