/**
 * WorkloadBalancingServerlessScheduler.h
 * This scheduler balances the workload across compute nodes based on invocation time limits
 * to minimize the overall makespan, assuming offline scheduling.
 */

#ifndef WRENCH_WORKLOAD_BALANCING_SERVERLESS_SCHEDULER_H
#define WRENCH_WORKLOAD_BALANCING_SERVERLESS_SCHEDULER_H

#include <wrench/services/compute/serverless/ServerlessScheduler.h>
#include <vector>
#include <unordered_map>

namespace wrench {
    /**
    * @brief A class that implements a very experimental/untested scheduler that attempts
    *        to make good load-balancing decisions.
    */
    class WorkloadBalancingServerlessScheduler : public ServerlessScheduler {
    public:
        WorkloadBalancingServerlessScheduler() = default;

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        ~WorkloadBalancingServerlessScheduler() override = default;

        std::shared_ptr<SchedulingDecisions> schedule(
            const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
            const std::shared_ptr<ServerlessStateOfTheSystem>& state
        ) override;

    private:
        void makeImageDecisions(const std::shared_ptr<SchedulingDecisions>& decisions,
                                const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
                                const std::shared_ptr<ServerlessStateOfTheSystem>& state);

        void makeInvocationDecisions(const std::shared_ptr<SchedulingDecisions>& decisions,
                                     const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
                                     const std::shared_ptr<ServerlessStateOfTheSystem>& state);


        // Function type -> total workload (in time units)
        std::unordered_map<std::string, double> function_workloads;

        // Function type -> count of pending invocations
        std::unordered_map<std::string, int> function_pending_count;

        // Node -> function -> cores allocated
        std::unordered_map<std::string, std::unordered_map<std::string, unsigned>> allocation_plan;

        // Helper to calculate workloads for each function type
        void calculateFunctionWorkloads(const std::vector<std::shared_ptr<Invocation>>& invocations);

        // Helper to create allocation plan
        void createAllocationPlan(const std::shared_ptr<ServerlessStateOfTheSystem>& state);

        // Map function names to their image files
        std::unordered_map<std::string, std::shared_ptr<DataFile>> function_images;

        /***********************/
        /** \endcond          **/
        /***********************/
    };
} // namespace wrench

#endif // WRENCH_WORKLOAD_BALANCING_SERVERLESS_SCHEDULER_H
