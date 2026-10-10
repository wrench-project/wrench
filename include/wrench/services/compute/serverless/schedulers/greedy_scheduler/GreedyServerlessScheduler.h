#ifndef WRENCH_GREEDYSERVERLESSCHEDULER_H
#define WRENCH_GREEDYSERVERLESSCHEDULER_H

#include <wrench/services/compute/serverless/schedulers/ServerlessScheduler.h>
#include <wrench/services/compute/serverless/schedulers/ServerlessSchedulingState.h>
#include <wrench/services/compute/serverless/schedulers/greedy_scheduler/invocation_sorting_policies/ServerlessInvocationOrderingPolicy.h>
#include <wrench/services/compute/serverless/schedulers/greedy_scheduler/node_selection_policies/ServerlessNodeSelectionPolicy.h>
#include <wrench/services/compute/serverless/schedulers/greedy_scheduler/eviction_policies/ServerlessEvictionPolicy.h>
#include <wrench/services/compute/serverless/schedulers/greedy_scheduler/plan_selection_policies/ServerlessPlanSelectionPolicy.h>

namespace wrench {
    class ServerlessSchedulingState;
}

namespace wrench {
    /**
     * @brief A class that implements a greedy scheduler for a serverless compute service. It can be
     * instantiated with various policies for ordering schedulable invocations, deciding on
     * evictions (idle containers, in-RAM layers, on-disk layers), and comparing various action/evication
     * plans on different compute nodes. Some policies are provided, but custom one can be developed.
     *
     * Note that this scheduling algorithm, while reasonable, is not necessarily in line with the
     * scheduling in real-world serverless platforms.
     */
    class GreedyServerlessScheduler : public ServerlessScheduler {

    public:

        GreedyServerlessScheduler(
            std::shared_ptr<ServerlessInvocationOrderingPolicy> invocation_ordering,
            std::shared_ptr<ServerlessNodeSelectionPolicy> node_selection,
            std::shared_ptr<ServerlessEvictionPolicy> eviction,
            std::shared_ptr<ServerlessPlanSelectionPolicy> plan_selection);

        [[nodiscard]] ServerlessInvocationOrderingPolicy* getInvocationOrderingPolicy() const;
        [[nodiscard]] ServerlessNodeSelectionPolicy* getNodeSelectionPolicy() const;
        [[nodiscard]] ServerlessEvictionPolicy* getEvictionPolicy() const;
        [[nodiscard]] ServerlessPlanSelectionPolicy* getPlanSelectionPolicy() const;

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        ~GreedyServerlessScheduler() override = default;

        std::shared_ptr<ServerlessSchedulingDecisions> schedule(
            const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
            const ServerlessStateOfTheSystem* state) override;

    protected:
        std::pair<std::shared_ptr<ServerlessComputeNode>, std::shared_ptr<ServerlessSchedulingDecisions>>
        pickComputeNode(
            const std::shared_ptr<ServerlessSchedulingState>& scheduling_state,
            const std::shared_ptr<Invocation>& inv);

        [[nodiscard]] std::shared_ptr<ServerlessSchedulingDecisions> makeEvictionPlan(
            const std::shared_ptr<ServerlessComputeNode>& node,
            const std::shared_ptr<ServerlessSchedulingState>& scheduling_state,
            sg_size_t needed_ram,
            sg_size_t needed_disk,
            const std::set<std::shared_ptr<ImageLayer>>& invocation_layers_to_protect) const;


	    /** @brief The invocation ordering policy **/
        std::shared_ptr<ServerlessInvocationOrderingPolicy> _invocation_ordering_policy;
        /** @brief The node selection policy **/
        std::shared_ptr<ServerlessNodeSelectionPolicy> _node_selection_policy;
	    /** @brief The eviction policy **/
        std::shared_ptr<ServerlessEvictionPolicy> _eviction_policy;
	    /** @brief The plan selection policy **/
        std::shared_ptr<ServerlessPlanSelectionPolicy> _plan_selection_policy;

        /***********************/
        /** \endcond          **/
        /***********************/
    };
} // namespace wrench

#endif //WRENCH_GREEDYSERVERLESSCHEDULER_H
