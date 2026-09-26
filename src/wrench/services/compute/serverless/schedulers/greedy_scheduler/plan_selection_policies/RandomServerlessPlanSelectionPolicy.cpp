#include <wrench/services/compute/serverless/schedulers/greedy_scheduler/plan_selection_policies/RandomServerlessPlanSelectionPolicy.h>

#include "wrench/services/compute/serverless/schedulers/ServerlessScheduler.h"

namespace wrench {

    /**
     * @brief Constructor
     * @param seed The seed of the RNG
     */
    RandomServerlessPlanSelectionPolicy::RandomServerlessPlanSelectionPolicy(const unsigned int seed) {
        _rng = std::mt19937(seed);
    }

    /**
     * @brief Select the node that has the best (generic) plan
     * @param plans a map of invocation plans
     * @return a compute node
     */
    std::shared_ptr<ServerlessComputeNode> RandomServerlessPlanSelectionPolicy::pickBestPlan(
        const std::map<std::shared_ptr<ServerlessComputeNode>, std::shared_ptr<ServerlessSchedulingDecisions>>& plans) {

        if (plans.empty()) {
            return nullptr;
        }

        std::vector<std::shared_ptr<ServerlessComputeNode>> nodes;
        nodes.reserve(plans.size());

        for (const auto& [node, plan] : plans) {
            nodes.push_back(node);
        }

        // Establish an ordering independent of pointer addresses.
        // Assumes hostnames uniquely identify these nodes.
        std::sort(
            nodes.begin(), nodes.end(),
            [](const auto& a, const auto& b) {
                return a->hostname < b->hostname;
            });

        std::uniform_int_distribution<std::size_t> distribution(
            0, nodes.size() - 1);

        return nodes[distribution(_rng)];
    }


    /**
    * @brief Select the node that has the best invocation plan
    * @param plans a map of plans
    * @return a compute node
    */
    std::shared_ptr<ServerlessComputeNode> RandomServerlessPlanSelectionPolicy::pickBestInvocationPlan(
        const std::map<std::shared_ptr<ServerlessComputeNode>, std::shared_ptr<ServerlessSchedulingDecisions>>&
        plans) {
        return pickBestPlan(plans);
    }

    /**
    * @brief Select the node that has the best layer-load plan
    * @param plans a map of plans
    * @return a compute node
    */
    std::shared_ptr<ServerlessComputeNode> RandomServerlessPlanSelectionPolicy::pickBestLayerLoadPlan(
        const std::map<std::shared_ptr<ServerlessComputeNode>, std::shared_ptr<ServerlessSchedulingDecisions>>&
        plans) {
        return pickBestPlan(plans);
    }

    /**
    * @brief Select the node that has the best layer-copy plan
    * @param plans a map of plans
    * @return a compute node
    */
    std::shared_ptr<ServerlessComputeNode> RandomServerlessPlanSelectionPolicy::pickBestLayerCopyPlan(
        const std::map<std::shared_ptr<ServerlessComputeNode>, std::shared_ptr<ServerlessSchedulingDecisions>>&
        plans) {
        return pickBestPlan(plans);
    }
}
