/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/workflow/parallel_model/ConstantEfficiencyParallelModel.h>

#include "wrench/logging/TerminalOutput.h"

WRENCH_LOG_CATEGORY(wrench_core_constant_efficiency_parallel_model, "Log category for ConstantEfficiencyParallelModel");

namespace wrench {

    /**
     * @brief Constructor
     * @param efficiency: parallel efficiency (between 0.0 and 1.0).
     */
    ConstantEfficiencyParallelModel::ConstantEfficiencyParallelModel(double efficiency) {
        if (efficiency < 0.0 or efficiency > 1.0) {
            throw std::invalid_argument("ConstantEfficiencyParallelModel::ConstantEfficiencyParallelModel(): "
                                     "Invalid efficiency argument (must be between 0.0 and 1.0)");
        }
        this->efficiency = efficiency;
    }

    /**
     * @brief Get the model's parallel efficiency
     * @return a parallel efficiency
     */
    double wrench::ConstantEfficiencyParallelModel::getEfficiency() {
        return this->efficiency;
    }

    /**
     * @brief Returns the amount of work each thread much perform
     * @param total_work: total amount of work
     * @param num_threads: the number of threads
     * @return a vector of work amounts
     */
    std::vector<double> ConstantEfficiencyParallelModel::getWorkPerThread(double total_work, unsigned long num_threads) {

        double thread_work = (total_work) / ((double)num_threads * this->efficiency);
        std::vector<double> work_per_threads;
        for (unsigned int i=0; i < num_threads; i++) {
            work_per_threads.push_back(thread_work);
        }
        return work_per_threads;
    }
}
