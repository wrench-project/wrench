/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "wrench/logging/TerminalOutput.h"

#include "wrench/services/helper_services/action_executor/ActionExecutor.h"
#include "wrench/services/helper_services/action_executor/ComputeActionExecutor.h"
#include "wrench/action/ComputeAction.h"
#include "wrench/simulation/Simulation.h"
#include "wrench/services/helper_services/action_executor/ActionExecutorMessage.h"

WRENCH_LOG_CATEGORY(wrench_core_compute_action_executor,"Log category for Compute Action Executor");

//#define S4U_JOIN_WORKS

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the workunit execution will run
     * @param num_cores: the number of cores available to the workunit executor
     * @param ram_utilization: the number of bytes of RAM used by the service
     * @param callback_mailbox: the callback mailbox to which a "work done" or "work failed" message will be sent
     * @param action: the action to perform
     * @param thread_startup_overhead: the thread_startup overhead, in seconds
     * @param simulate_computation_as_sleep: simulate computation as a sleep instead of an actual compute thread (for simulation scalability reasons)
     */
    ComputeActionExecutor::ComputeActionExecutor(
            std::string hostname,
            unsigned long num_cores,
            double ram,
            std::string callback_mailbox,
            std::shared_ptr<ComputeAction> action,
            double thread_startup_overhead,
            bool simulate_computation_as_sleep) :
            ActionExecutor(hostname,
                           callback_mailbox,
                           std::dynamic_pointer_cast<Action>(action)) {

        if (num_cores < 1) {
            throw std::invalid_argument("ComputeActionExecutor::ComputeActionExecutor(): num_cores must be >= 1");
        }
        if (ram < 0) {
            throw std::invalid_argument("ComputeActionExecutor::ComputeActionExecutor(): ram must be >= 0");
        }
        if (action == nullptr) {
            throw std::invalid_argument("ComputeActionExecutor::ComputeActionExecutor(): action cannot be nullptr");
        }
        if (thread_startup_overhead < 0) {
            throw std::invalid_argument("ComputeActionExecutor::ComputeActionExecutor(): thread_startup_overhead must be >= 0");
        }

        if (ram < action->getRAM() or num_cores < action->getMinNumCores() or num_cores > action->getMaxNumCores()) {
            throw std::runtime_error("ComputeAction::execute(): Invalid resource specifications - thus should not happen");
        }

        this->callback_mailbox = callback_mailbox;
        this->action = action;
        this->thread_startup_overhead = thread_startup_overhead;
        this->simulate_computation_as_sleep = simulate_computation_as_sleep;
        this->num_cores = num_cores;
        this->ram = ram;
    }

    /**
     * @brief Kill the worker thread
     *
     * @param job_termination: if the reason for being killed is that the job was terminated by the submitter
     * (as opposed to being terminated because the above service was also terminated).
     */
    void ComputeActionExecutor::kill(bool job_termination) {
        this->acquireDaemonLock();
        for (auto const &ct : this->compute_threads) {
            // Should work even if already dead
            ct->kill();
        }
        this->killActor();
        this->killed_on_purpose = job_termination;
        this->releaseDaemonLock();
    }

    /**
    * @brief Main method of the action exedcutor
    *
    * @return 1 if a failure timestamp should be generated, 0 otherwise
    *
    * @throw std::runtime_error
    */
    int ComputeActionExecutor::main() {

        S4U_Simulation::computeZeroFlop(); // to block in case pstate speed is 0

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_BLUE);WRENCH_INFO(
                "New Action Executor starting (%s) to do action %s",
                this->getName().c_str(), this->action->getName().c_str());

        SimulationMessage *msg_to_send_back = nullptr;

        // Compute the amount of work per thread
        auto compute_action = std::dynamic_pointer_cast<ComputeAction>(this->action);
        std::vector<double> work_per_thread = compute_action->getParallelModel()->getWorkPerThread(
                compute_action->getFlops(), num_cores);

        // Simulate as sleep?
        try {
            if (simulate_computation_as_sleep) {
                simulateComputationAsSleep(work_per_thread);
            } else {
                simulateComputationWithComputeThreads(work_per_thread);
            }
            msg_to_send_back = new ActionExecutorDoneMessage(this->getSharedPtr<ActionExecutor>());
        } catch (ExecutionException &e) {
            this->action->setFailureCause(e.getCause());
            msg_to_send_back = new ActionExecutorDoneMessage(this->getSharedPtr<ActionExecutor>());
        }

        // Send callback
        try {
            S4U_Mailbox::putMessage(this->callback_mailbox, msg_to_send_back);
        } catch (std::shared_ptr <NetworkError> &cause) {
            WRENCH_INFO("Action executor can't report back due to network error.. oh well!");
        }
        return 0;
    }

    /**
     * @brief Simulate computation with a single sleep
     *
     * @param work_per_thread: amount of work (in flop) that each thread should do
     */
    void ComputeActionExecutor::simulateComputationAsSleep(std::vector<double> &work_per_thread) {
        double max_work_per_thread = *(std::max_element(work_per_thread.begin(), work_per_thread.end()));
        // Thread startup_overhead
        S4U_Simulation::sleep((double) this->num_cores * this->thread_startup_overhead);
        // Then sleep for the computation duration
        double sleep_time = max_work_per_thread / Simulation::getFlopRate();
        Simulation::sleep(sleep_time);
    }

    /**
     * @brief Simulation computation with compute threads
     *
     * @param work_per_thread: amount of work (in flop) that each thread should do
     */
    void ComputeActionExecutor::simulateComputationWithComputeThreads(vector<double> &work_per_thread) {

        std::string tmp_mailbox = S4U_Mailbox::generateUniqueMailboxName("compute_action_executor");



        WRENCH_INFO("Launching %ld compute threads", this->num_cores);
        // Create a compute thread to run the computation on each core
        bool success = true;
        for (auto const &work : work_per_thread) {
            try {
                S4U_Simulation::sleep(this->thread_startup_overhead);
            } catch (std::exception &e) {
                WRENCH_INFO("Got an exception while sleeping... perhaps I am being killed?");
                throw ExecutionException(std::shared_ptr<FailureCause>(new FatalFailure()));
            }
            std::shared_ptr <ComputeThread> compute_thread;
            try {
                // Nobody kills me while I am starting a compute threads!
                this->acquireDaemonLock();
                compute_thread = std::shared_ptr<ComputeThread>(
                        new ComputeThread(S4U_Simulation::getHostName(), work, tmp_mailbox));
                compute_thread->simulation = this->simulation;
                compute_thread->start(compute_thread, true, false); // Daemonized, no auto-restart
                this->releaseDaemonLock();
            } catch (std::exception &e) {
                WRENCH_INFO("Could not create compute thread... perhaps I am being killed?");
                success = false;
                this->releaseDaemonLock();
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
        // Wait for all compute threads to complete
#ifndef S4U_JOIN_WORKS
        for (unsigned long i = 0; i < this->compute_threads.size(); i++) {
            WRENCH_INFO("Waiting for message from a compute threads...");
            try {
                S4U_Mailbox::getMessage(tmp_mailbox);
            } catch (std::shared_ptr <NetworkError> &e) {
                WRENCH_INFO("Got a network error when trying to get completion message from compute thread");
                // Do nothing, perhaps the child has died
                success = false;
                continue;
            }
            WRENCH_INFO("Got it...");
        }
#else
        for (unsigned long i=0; i < this->compute_threads.size(); i++) {
            WRENCH_INFO("JOINING WITH A COMPUTE THREAD %s", this->compute_threads[i]->process_name.c_str());
          try {
            this->compute_threads[i]->join();
          } catch (std::shared_ptr<FatalFailure> &e) {
            WRENCH_INFO("EXCEPTION WHILE JOINED");
            // Do nothing, perhaps the child has died...
            continue;
          }
          WRENCH_INFO("JOINED with COMPUTE THREAD %s", this->compute_threads[i]->process_name.c_str());

        }
#endif
        WRENCH_INFO("All compute threads have completed");
        this->compute_threads.clear();

        if (!success) {
            throw ExecutionException(std::shared_ptr<FailureCause>(new ComputeThreadHasDied()));
        }
    }

    /**
     * @brief Cleanup method
     *
     * @param has_returned_from_main: whether main() returned
     * @param return_value: the return value (if main() returned)
     */
    void ComputeActionExecutor::cleanup(bool has_returned_from_main, int return_value) {
        WRENCH_DEBUG(
                "In on_exit.cleanup(): ComputeActionExecutor: %s has_returned_from_main = %d (return_value = %d, killed_on_pupose = %d)",
                this->getName().c_str(), has_returned_from_main, return_value,
                this->killed_on_purpose);

        this->action->setEndDate(S4U_Simulation::getClock());

        // Check if it's a failure/termination!
        if (not has_returned_from_main) {
            if (this->killed_on_purpose) {
                this->action->setState(Action::State::KILLED);
            } else {
                this->action->setState(Action::State::FAILED);
            }
        } else {
            this->action->setState(Action::State::COMPLETED);
        }
    }

}
