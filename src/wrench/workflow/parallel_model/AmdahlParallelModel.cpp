/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/workflow/parallel_model/AmdahlParallelModel.h>

#include <wrench/logging/TerminalOutput.h>

WRENCH_LOG_CATEGORY(wrench_core_amdahl_parallel_model, "Log category for AmdahlParallelModel");

namespace wrench {

    /**
     * @brief Constructor
     * @param alpha: the fraction (i.e., a number between 0.0 and 1.0) of the task's
     *                work that is perfectly parallelizable. Setting this value to 0 means
     *                that the task is purely sequential, and setting it to 1 means that the
     *                task is perfectly parallelizable.
     */
    AmdahlParallelModel::AmdahlParallelModel(double alpha) {
        if (alpha < 0.0 or alpha > 1.0) {
            throw std::invalid_argument("AmdahlParallelModel::AmdahlParallelModel(): "
                                        "Invalid alpha argument (must be between 0.0 and 1.0)");
        }
        this->alpha = alpha;
    }

    /**
     * @brief Get the
     * @return
     */
    double AmdahlParallelModel::getAlpha() {
        return this->alpha;
    }

    //    /**
    //     * @brief Returns the amount of work each thread much perform
    //     * @param total_work: total amount of work
    //     * @param num_threads: the number of threads
    //     * @return a vector of work amounts
    //     */
    //    std::vector<double> AmdahlParallelModel::getWorkPerThread(double total_work, unsigned long num_threads) {
    //
    //        double sequential_work = (1 - this->alpha) * total_work;
    //        double per_thread_parallel_work = (total_work - sequential_work) / (double)num_threads;
    //        std::vector<double> work_per_threads;
    //        for (unsigned int i=0; i < num_threads; i++) {
    //            double work = per_thread_parallel_work + (i == 0 ? sequential_work : 0.0);
    //            work_per_threads.push_back(work);
    //        }
    //        return work_per_threads;
    //    }

    double AmdahlParallelModel::getPurelySequentialWork(double total_work, unsigned long num_threads) {
        return (1 - this->alpha) * total_work;
    }

    double AmdahlParallelModel::getParallelPerThreadWork(double total_work, unsigned long num_threads) {
        return (total_work * this->alpha) / (double) num_threads;
    }
}// namespace wrench
