/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/workflow/parallel_model/CustomParallelModel.h>

#include "wrench/logging/TerminalOutput.h"

WRENCH_LOG_CATEGORY(wrench_core_custom_parallel_model, "Log category for CustomParallelModel");

namespace wrench {

    /**
     * @brief Constructor
     * @param lambda: a function that, when given a total flop amount and a number of
     *        threads, returns a vector of per-thread work amounts
     */
    CustomParallelModel::CustomParallelModel(std::function<std::vector<double>(double, unsigned long)> lambda) {
        this->lambda = lambda;
    }

    /**
     * @brief Returns the amount of work each thread much perform
     * @param total_work: total amount of work
     * @param num_threads: the number of threads
     * @return a vector of work amounts
     */
    std::vector<double> CustomParallelModel::getWorkPerThread(double total_work, unsigned long num_threads) {
        return this->lambda(total_work, num_threads);
    }
}