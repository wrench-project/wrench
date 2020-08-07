/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "wrench/workflow/parallel_model/ParallelModel.h"
#include "wrench/workflow/parallel_model/AmdahlParallelModel.h"
#include "wrench/workflow/parallel_model/ConstantEfficiencyParallelModel.h"
#include "wrench/workflow/parallel_model/CustomParallelModel.h"

#include "wrench/logging/TerminalOutput.h"

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
     * @param lambda: a function that, when given a total flop amount and a number of
     *        threads, returns a vector of per-thread work amounts
     *
     * @return a model instance
     **/
    std::shared_ptr<ParallelModel> ParallelModel::CUSTOM(std::function<std::vector<double>(double, long)> lambda) {
        return std::shared_ptr<ParallelModel>(new CustomParallelModel(lambda));
    }

};

