/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_SERVERLESS_EXAMPLE_CONTROLLER_H
#define WRENCH_SERVERLESS_EXAMPLE_CONTROLLER_H

#include <wrench-dev.h>

namespace wrench {

    class Simulation;

    /**
     *  @brief An execution controller implementation
     */
    class ServerlessExampleExecutionController : public ExecutionController {

    public:
        // Constructor
        ServerlessExampleExecutionController(
                std::shared_ptr<ServerlessComputeService> compute_service,
                const std::string &hostname);

    protected:

    private:
        // main() method of the WMS
        int main() override;

        const std::shared_ptr<ServerlessComputeService> compute_service;
    };
}// namespace wrench
#endif//WRENCH_SERVERLESS_EXAMPLE_CONTROLLER_H
