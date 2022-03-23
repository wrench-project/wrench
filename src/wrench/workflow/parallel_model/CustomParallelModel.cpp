/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/workflow/parallel_model/CustomParallelModel.h>

#include <wrench/logging/TerminalOutput.h>

WRENCH_LOG_CATEGORY(wrench_core_custom_parallel_model, "Log category for CustomParallelModel");

namespace wrench {

    /**
     * @brief Constructor
     * @param lambda: a function that, when given a total flop amount and a number of
     *        threads, the amount of purely sequential work
     */
    CustomParallelModel::CustomParallelModel(std::function<double(double, unsigned long)> lambda_sequential,
                                             std::function<double(double, unsigned long)> lambda_per_thread) {
        this->lambda_sequential = lambda_sequential;
        this->lambda_per_thread = lambda_per_thread;
    }

//    /**
//     * @brief Returns the amount of work each thread much perform
//     * @param total_work: total amount of work
//     * @param num_threads: the number of threads
//     * @return a vector of work amounts
//     */
//    std::vector<double> CustomParallelModel::getWorkPerThread(double total_work, unsigned long num_threads) {
//        return this->lambda(total_work, num_threads);
//    }

    /**
     * @brief Returns the purely sequential amount of work
     * @param total_work: total amount of work (in flops)
     * @param num_threads: number of threads
     * @return an amount of work (in flops)
     */
    double CustomParallelModel::getPurelySequentialWork(double total_work, unsigned long num_threads) {
        return this->lambda_sequential(total_work, num_threads);
    }

    double CustomParallelModel::getParallelPerThreadWork(double total_work, unsigned long num_threads) {
        return this->lambda_per_thread(total_work, num_threads);
    }

}