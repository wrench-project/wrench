/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/logging/TerminalOutput.h>
#include <wrench/simulation/Simulation.h>
#include <wrench/action/Action.h>
#include <wrench/action/ComputeAction.h>
#include <wrench/services/helper_services/action_executor/ActionExecutor.h>
#include <wrench/failure_causes/ComputationHasDied.h>
#include <wrench/failure_causes/FatalFailure.h>
#include <wrench/exceptions/ExecutionException.h>

#include <utility>

WRENCH_LOG_CATEGORY(wrench_compute_action, "Log category for Compute Action");

namespace wrench {

    /**
     * @brief Constructor
     * @param name: the action's name (if empty, a unique name will be picked)
     * @param job: the job this action belongs to
     * @param flops: the number of flops to perform
     * @param ram: the ram that is required
     * @param min_num_cores: the minimum number of cores that can be used
     * @param max_num_cores: the maximum number of cores that can be used
     * @param parallel_model: the parallel model (to determine speedup vs. number of cores)
     */
    ComputeAction::ComputeAction(const std::string &name,
                                 std::shared_ptr<CompoundJob> job,
                                 double flops,
                                 double ram,
                                 unsigned long min_num_cores,
                                 unsigned long max_num_cores,
                                 std::shared_ptr<ParallelModel> parallel_model) : Action(name, "compute_", std::move(job)) {
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
     * @brief Returns the action's minimum required memory footprint
     * @return a number of bytes
     */
    double ComputeAction::getMinRAMFootprint() const {
        return this->ram;
    }

    /**
     * @brief Returns the action's parallel model
     * @return a parallel model
     */
    std::shared_ptr<ParallelModel> ComputeAction::getParallelModel() const {
        return this->parallel_model;
    }

    /**
     * @brief Method to execute the task
     * @param action_executor: the executor that executes this action
     */
    void ComputeAction::execute(std::shared_ptr<ActionExecutor> action_executor) {
        auto num_threads = action_executor->getNumCoresAllocated();
        if ((num_threads < this->min_num_cores) || (num_threads > this->max_num_cores) || (action_executor->getMemoryAllocated() < this->ram)) {
            throw ExecutionException(std::shared_ptr<FailureCause>(new FatalFailure("Invalid resource specs for Action Executor")));
        }

        double sequential_work = this->getParallelModel()->getPurelySequentialWork(this->getFlops(), num_threads);
        double parallel_per_thread_work = this->getParallelModel()->getParallelPerThreadWork(this->getFlops(), num_threads);
        if (action_executor->getSimulateComputationAsSleep()) {
            this->simulateComputationAsSleep(action_executor, num_threads, sequential_work, parallel_per_thread_work);
        } else {
            this->simulateComputationAsComputation(action_executor, num_threads, sequential_work,
                                                   parallel_per_thread_work);
        }
    }

    /**
     * @brief Method called when the task terminates
     * @param action_executor:  the executor that executes this action
     */
    void ComputeAction::terminate(std::shared_ptr<ActionExecutor> action_executor) {
#if 0
        action_executor->acquireDaemonLock();
        for (auto const &ct : this->compute_threads) {
            // Should work even if already dead
            ct->kill();
        }
        action_executor->releaseDaemonLock();
#endif
    }

    /**
  * @brief Simulate computation with a single sleep
  * @param action_executor:  the executor that executes this action
  * @param num_threads: the number of threads
  * @param sequential_work: the total amount of sequential work
  * @param parallel_per_thread_work: the parallel per-thread work
  */
    void ComputeAction::simulateComputationAsSleep(const std::shared_ptr<ActionExecutor> &action_executor, unsigned long num_threads, double sequential_work, double parallel_per_thread_work) {
        // Thread startup_overhead
        S4U_Simulation::sleep((double) (num_threads) *action_executor->getThreadCreationOverhead());
        // Then sleep for the computation duration
        double sleep_time = (sequential_work + parallel_per_thread_work) / Simulation::getFlopRate();
        Simulation::sleep(sleep_time);
    }

    /**
     * @brief Simulation computation with compute threads
     * @param action_executor:  the executor that executes this action
     * @param num_threads: the number of threads to use
     * @param sequential_work: total amount of sequential work
     * @param parallel_per_thread_work: amount of parallel per-thread work
     */
    void ComputeAction::simulateComputationAsComputation(const std::shared_ptr<ActionExecutor> &action_executor, unsigned long num_threads, double sequential_work, double parallel_per_thread_work) {

        try {
            S4U_Simulation::compute_multi_threaded(num_threads,
                                                   action_executor->getThreadCreationOverhead(),
                                                   sequential_work,
                                                   parallel_per_thread_work);
        } catch (std::exception &e) {
            throw ExecutionException(std::shared_ptr<FailureCause>(new ComputationHasDied()));
        }
        WRENCH_INFO("All compute threads have completed successfully");
    }

}// namespace wrench
