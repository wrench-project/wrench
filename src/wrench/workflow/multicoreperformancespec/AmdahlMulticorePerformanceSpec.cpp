/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/workflow/multicoreperformancespec/AmdahlMulticorePerformanceSpec.h>

#include "wrench/workflow/multicoreperformancespec/MulticorePerformanceSpec.h"

#include "wrench/logging/TerminalOutput.h"

WRENCH_LOG_CATEGORY(wrench_core_amdahlmulticorePerformanceSpec, "Log category for AmdahlMulticorePerformanceSpec");

namespace wrench {

    /**
     * @brief Constructor
     * @param alpha: the fraction (i.e., a number between 0.0 and 1.0) of the task's
     *                work that is perfectly parallelizable.
     */
    AmdahlMulticorePerformanceSpec::AmdahlMulticorePerformanceSpec(double alpha) {
        if (alpha < 0.0 or alpha > 1.0) {
            throw std::runtime_error("AmdahlMulticorePerformanceSpec::AmdahlMulticorePerformanceSpec(): "
                                     "Invalid alpha argument (must be between 0.0 and 1.0)");
        }
        this->alpha = alpha;
    }

    /**
     * @brief Returns the amount of work each thread much perform
     * @param numThreads: the number of threads
     * @return a vector of work amounts
     */
    std::vector<double> AmdahlMulticorePerformanceSpec::getWorkPerThread(unsigned long num_threads) {

        double total_work = this->task->getFlops();
        double sequential_work = (1 - this->alpha) * total_work;
        double per_thread_parallel_work = (total_work - sequential_work) / (double)num_threads;
        std::vector<double> work_per_threads;
        for (int i=0; i < num_threads; i++) {
            double work = per_thread_parallel_work + (i == 0 ? sequential_work : 0.0);
            work_per_threads.push_back(work);
        }
        return work_per_threads;
    }
}