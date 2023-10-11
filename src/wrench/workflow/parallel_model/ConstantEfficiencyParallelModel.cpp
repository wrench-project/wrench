/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/workflow/parallel_model/ConstantEfficiencyParallelModel.h>

#include <wrench/logging/TerminalOutput.h>

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
     * @return a parallel efficiency (a number between 0.0 and 1.0)
     */
    double wrench::ConstantEfficiencyParallelModel::getEfficiency() const {
        return this->efficiency;
    }

    /**
     * @brief Set the model's parallel efficiency
     * @param efficiency: a parallel efficiency (a number between 0.0 and 1.0)
     */
    void wrench::ConstantEfficiencyParallelModel::setEfficiency(double efficiency) {
        this->efficiency = efficiency;
    }

    //    /**
    //     * @brief Returns the amount of work each thread much perform
    //     * @param total_work: total amount of work
    //     * @param num_threads: the number of threads
    //     * @return a vector of work amounts
    //     */
    //    std::vector<double> ConstantEfficiencyParallelModel::getWorkPerThread(double total_work, unsigned long num_threads) {
    //
    //        double thread_work = (total_work) / ((double)num_threads * this->efficiency);
    //        std::vector<double> work_per_threads;
    //        for (unsigned int i=0; i < num_threads; i++) {
    //            work_per_threads.push_back(thread_work);
    //        }
    //        return work_per_threads;
    //    }


    /**
     * @brief Returns the purely sequential amount of work
     * @param total_work: total amount of work (in flops)
     * @param num_threads: number of threads
     * @return an amount of work (in flops)
     */
    double ConstantEfficiencyParallelModel::getPurelySequentialWork(double total_work, unsigned long num_threads) {
        return 0;
    }

    /**
     * @brief Returns the parallel per-thread amount of work
     * @param total_work: total amount of work (in flops)
     * @param num_threads: number of threads
     * @return an amount of work (in flops)
     */
    double ConstantEfficiencyParallelModel::getParallelPerThreadWork(double total_work, unsigned long num_threads) {
        return (total_work) / ((double) num_threads * this->efficiency);
    }

}// namespace wrench
