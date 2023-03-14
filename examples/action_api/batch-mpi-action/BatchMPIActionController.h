/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_EXAMPLE_MPI_ACTION_CONTROLLER_H
#define WRENCH_EXAMPLE_MPI_ACTION_CONTROLLER_H

#include <wrench-dev.h>

namespace wrench {

    class Simulation;

    /**
     *  @brief A Controller implementation (inherits from ExecutionController)
     */
    class BatchMPIActionController : public ExecutionController {

    public:
        // Constructor
        BatchMPIActionController(std::shared_ptr<BatchComputeService> batch_cs,
                                 std::shared_ptr<StorageService> ss,
                                 const std::string &hostname);

    private:
        // main() method of the Controller
        int main() override;

        std::shared_ptr<BatchComputeService> batch_cs;
        std::shared_ptr<StorageService> ss;
    };
}// namespace wrench
#endif//WRENCH_EXAMPLE_MPI_ACTION_CONTROLLER_H
