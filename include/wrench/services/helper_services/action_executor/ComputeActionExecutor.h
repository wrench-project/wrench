/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_COMPUTE_ACTION_EXECUTOR_H
#define WRENCH_COMPUTE_ACTION_EXECUTOR_H

#include <set>
//#include <wrench/services/helper_services/compute_thread/ComputeThread.h>
#include <wrench/services/helper_services/action_executor/ActionExecutor.h>


namespace wrench {

    class Simulation;
    class Action;
    class ComputeAction;
    class ComputeThread;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief An service that performs a WorkUnit
     */
    class ComputeActionExecutor : public ActionExecutor {

    public:

        ComputeActionExecutor(
                     std::string hostname,
                     unsigned long num_cores,
                     double ram,
                     std::string callback_mailbox,
                     std::shared_ptr<ComputeAction> action,
                     double thread_startup_overhead,
                     bool simulate_computation_as_sleep);

        std::shared_ptr<Action> getAction();
        void kill(bool job_termination) override;

    private:

        std::shared_ptr<Action> action;

        int main() override;
        void cleanup(bool has_returned_from_main, int return_value) override;

        void simulateComputationAsSleep(std::vector<double> &work_per_thread);
        void simulateComputationWithComputeThreads(std::vector<double> &work_per_thread);

        std::string callback_mailbox;
        unsigned long num_cores;
        double ram;
        double thread_startup_overhead;
        bool simulate_computation_as_sleep;

        std::vector<std::shared_ptr<ComputeThread>> compute_threads;

    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_COMPUTE_ACTION_EXECUTOR_H
