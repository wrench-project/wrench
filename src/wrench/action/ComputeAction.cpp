/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/simulation/Simulation.h"
#include "wrench/action/Action.h"
#include "wrench/action/ComputeAction.h"

namespace wrench {

    /**
     * @brief Constructor
     * @param name: the action's name (if empty, a unique name will be picked for you)
     * @param job: the job this action belongs to
     * @param flops: the number of flops to perform
     * @param ram: the ram that is required
     * @param min_num_cores: the minimum number of cores that can be used
     * @param max_num_cores: the maximum number of cores that can be used
     * @param parallel_model: the parallel model (to determine speedup vs. number of cores)
     */
    ComputeAction::ComputeAction(std::string name,
                                 std::shared_ptr<CompoundJob> job,
                                 double flops,
                                 double ram,
                                 unsigned long min_num_cores,
                                 unsigned long max_num_cores,
                                 std::shared_ptr<ParallelModel> parallel_model) : Action(name, job) {
        if ((flops < 0) || (min_num_cores < 1) || (max_num_cores < min_num_cores)) {
            throw std::invalid_argument("ComputeAction::ComputeAction(): invalid arguments");
        }
        this->flops = flops;
        this->min_num_cores = min_num_cores;
        this->max_num_cores = max_num_cores;
        this->ram = ram;
        this->parallel_model = std::move(parallel_model);
    }

    /**
     * @brief Returns the action's flops
     * @return a number of flops
     */
    double ComputeAction::getFlops() const {
        return this->flops;
    }

    /**
     * @brief Returns the action's minimum number of required cores
     * @return a number of cores
     */
    unsigned long ComputeAction::getMinNumCores() const {
        return this->min_num_cores;
    }

    /**
     * @brief Returns the action's maximum number of required cores
     * @return a number of cores
     */
    unsigned long ComputeAction::getMaxNumCores() const {
        return this->max_num_cores;
    }

    /**
     * @brief Returns the action's required RAM amount
     * @return a number of bytes
     */
    double ComputeAction::getRAM() const {
        return this->ram;
    }

    /**
     * @brief Returns the action's parallel model
     * @return a parallel model
     */
    std::shared_ptr<ParallelModel> ComputeAction::getParallelModel() const {
        return this->parallel_model;
    }

}
