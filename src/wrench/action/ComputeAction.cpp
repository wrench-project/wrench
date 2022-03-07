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
#include <wrench/services/helper_services/compute_thread//ComputeThread.h>
#include <wrench/failure_causes/ComputeThreadHasDied.h>
#include <wrench/failure_causes/FatalFailure.h>
#include <wrench/failure_causes/HostError.h>
#include <wrench/exceptions/ExecutionException.h>

WRENCH_LOG_CATEGORY(wrench_compute_action,"Log category for Compute Action");

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
    ComputeAction::ComputeAction(std::string name,
                                 std::shared_ptr<CompoundJob> job,
                                 double flops,
                                 double ram,
                                 unsigned long min_num_cores,
                                 unsigned long max_num_cores,
                                 std::shared_ptr<ParallelModel> parallel_model) : Action(name, "compute_",job) {
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

        std::vector<double> work_per_thread = this->getParallelModel()->getWorkPerThread(
                this->getFlops(), num_threads);
        if (this->simulate_computation_as_sleep) {
            this->simulateComputationAsSleep(action_executor, work_per_thread);
        } else {
            this->simulateComputationWithComputeThreads(action_executor, work_per_thread);
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
  * @param work_per_thread: amount of work (in flop) that each thread should do
  */
    void ComputeAction::simulateComputationAsSleep(std::shared_ptr<ActionExecutor> action_executor, std::vector<double> &work_per_thread) {
        double max_work_per_thread = *(std::max_element(work_per_thread.begin(), work_per_thread.end()));
        // Thread startup_overhead
        S4U_Simulation::sleep((double) (work_per_thread.size()) * this->thread_creation_overhead);
        // Then sleep for the computation duration
        double sleep_time = max_work_per_thread / Simulation::getFlopRate();
        Simulation::sleep(sleep_time);
    }

    /**
     * @brief Simulation computation with compute threads
     * @param action_executor:  the executor that executes this action
     *
     * @param work_per_thread: amount of work (in flop) that each thread should do
     */
    void ComputeAction::simulateComputationWithComputeThreads(std::shared_ptr<ActionExecutor> action_executor, vector<double> &work_per_thread) {

//        std::string tmp_mailbox = S4U_Mailbox::generateUniqueMailboxName("compute_action_executor");


        int num_threads = (int)work_per_thread.size();
        double max_work = *(std::max_element(work_per_thread.begin(), work_per_thread.end()));
        double min_work = *(std::min_element(work_per_thread.begin(), work_per_thread.end()));

        try {
            // Overhead
            S4U_Simulation::sleep(num_threads * this->thread_creation_overhead);
            if (num_threads == 1) {
                simgrid::s4u::this_actor::execute(max_work);
            } else {
                // Launch compute-heavy thread
                auto bottleneck_thread = simgrid::s4u::this_actor::exec_async(max_work);
                // Launch all other threads
                simgrid::s4u::this_actor::thread_execute(simgrid::s4u::this_actor::get_host(), min_work,
                                                         num_threads - 1);
                // Wait for the compute-heavy thread
                bottleneck_thread->wait();
            }
        } catch (simgrid::CancelException &e) {
            // I was killed
            throw ExecutionException(std::shared_ptr<FailureCause>(new ComputeThreadHasDied()));
        } catch (std::exception &e) {
            throw ExecutionException(std::shared_ptr<FailureCause>(new ComputeThreadHasDied()));
        }
        WRENCH_INFO("All compute threads have completed successfully");

#if 0
        this->compute_threads.clear();
        WRENCH_INFO("Launching %ld compute threads", work_per_thread.size());
        // Create a compute thread to run the computation on each core
        bool success = true;
        for (auto const &work : work_per_thread) {
            try {
                S4U_Simulation::sleep(this->thread_creation_overhead);
            } catch (std::exception &e) {
                WRENCH_INFO("Got an exception while sleeping... perhaps I am being killed?");
                throw ExecutionException(std::shared_ptr<FailureCause>(new FatalFailure("")));
            }
            std::shared_ptr <ComputeThread> compute_thread;
            try {
                // Nobody kills me while I am starting a compute threads!
                action_executor->acquireDaemonLock();
                compute_thread = std::shared_ptr<ComputeThread>(
                        new ComputeThread(S4U_Simulation::getHostName(), work, nullptr));
                compute_thread->setSimulation(action_executor->getSimulation());
                compute_thread->start(compute_thread, true, false); // Daemonized, no auto-restart
                action_executor->releaseDaemonLock();
            } catch (std::exception &e) {
                WRENCH_INFO("Could not create compute thread... perhaps I am being killed?");
                success = false;
                action_executor->releaseDaemonLock();
                break;
            }
            this->compute_threads.push_back(compute_thread);
        }

        if (not success) {
            WRENCH_INFO("Failed to create some compute threads...");
            // TODO: Dangerous to kill these now?? (this was commented out before, but seems legit, so Henri uncommented it)
            for (auto const &ct : this->compute_threads) {
                ct->kill();
            }
            throw ExecutionException(std::shared_ptr<FailureCause>(new ComputeThreadHasDied()));
        }

        success = true;
        // Wait for all compute threads to complete using a JOIN
        for (const auto & compute_thread : this->compute_threads) {
            std::pair<bool, int> thread_status;
            try {
                thread_status = compute_thread->join();
            } catch (std::shared_ptr<FatalFailure> &e) {
                success = false;
                continue;
            }
            bool has_returned_from_main = std::get<0>(thread_status);
            if (not has_returned_from_main) {
                success = false;
            } else {
            }
        }

        this->compute_threads.clear();

        if (!success) {
                        throw ExecutionException(std::shared_ptr<FailureCause>(new ComputeThreadHasDied()));
        }
                WRENCH_INFO("All compute threads have completed successfully");

#endif
    }

}
