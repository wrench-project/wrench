/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/workflow/parallel_model/ParallelModel.h>
#include <wrench/workflow/parallel_model/AmdahlParallelModel.h>
#include <wrench/workflow/parallel_model/ConstantEfficiencyParallelModel.h>
#include <wrench/workflow/parallel_model/CustomParallelModel.h>

#include <wrench/logging/TerminalOutput.h>

WRENCH_LOG_CATEGORY(wrench_core_parallel_model, "Log category for ParallelModel");

namespace wrench {

    /**
     * @brief Create an instance of an "Amdahl" parallel model
     * @param alpha: the fraction (i.e., a number between 0.0 and 1.0) of the task's
     *         work that is perfectly parallelizable. Setting this value to 0 means
     *         that the task is purely sequential, and setting it to 1 means that the
     *         task is perfectly parallelizable.
     *
     * @return a model instance
     **/
    std::shared_ptr<ParallelModel> ParallelModel::AMDAHL(double alpha) {
        return std::shared_ptr<ParallelModel>(new AmdahlParallelModel(alpha));
    }

    /**
     * @brief Create an instance of a "Constant Efficiency" parallel model
     * @param efficiency: the parallel efficiency (which does not depend on the number of threads/cores).
     *
     * @return a model instance
     **/
    std::shared_ptr<ParallelModel> ParallelModel::CONSTANTEFFICIENCY(double efficiency) {
        return std::shared_ptr<ParallelModel>(new ConstantEfficiencyParallelModel(efficiency));
    }

    /**
     * @brief Create an instance of a "Custom" parallel model
     * @param lambda_sequential: a function that, when given a total flop amount and a number of
     *        threads, returns the amount of purely sequential work, in flops
     * @param lambda_per_thread: a function that, when given a total flop amount and a number of
     *        threads, returns the amount of per-thread parallel work, in flops
     *
     * @return a model instance
     **/
    std::shared_ptr<ParallelModel> ParallelModel::CUSTOM(std::function<double(double, long)> lambda_sequential, std::function<double(double, long)> lambda_per_thread) {
        return std::shared_ptr<ParallelModel>(new CustomParallelModel(lambda_sequential, lambda_per_thread));
    }

};// namespace wrench
