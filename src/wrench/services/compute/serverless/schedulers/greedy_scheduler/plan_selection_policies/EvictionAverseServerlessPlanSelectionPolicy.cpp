#include <wrench/services/compute/serverless/schedulers/greedy_scheduler/plan_selection_policies/EvictionAverseServerlessPlanSelectionPolicy.h>

#include "wrench/services/compute/serverless/schedulers/ServerlessScheduler.h"

namespace wrench {
    /**
     * @brief Select the node that has the best (generic) plan
     * @param plans a map of invocation plans
     * @return a compute node
     */
    std::shared_ptr<ServerlessComputeNode> EvictionAverseServerlessPlanSelectionPolicy::pickBestPlan(
        const std::map<std::shared_ptr<ServerlessComputeNode>, std::shared_ptr<ServerlessSchedulingDecisions>>& plans) {

        if (plans.empty()) {
            return nullptr;
        }

        // Lower is better. Later criteria matter only when earlier ones tie.
        const auto rank =
            [](const std::shared_ptr<ServerlessSchedulingDecisions>& plan) {
            return std::make_tuple(
                plan->layer_evictions_from_disk.size(),
                plan->layer_evictions_from_ram.size(),
                plan->idle_container_terminations.size(),
                plan->image_layer_copies_to_disk.size(),
                plan->image_layer_loads_to_RAM.size());
        };

        std::shared_ptr<ServerlessComputeNode> picked_node;

        for (const auto& [node, plan] : plans) {
            if (!picked_node) {
                picked_node = node;
                continue;
            }

            const auto candidate_rank = rank(plan);
            const auto best_rank = rank(plans.at(picked_node));

            if (candidate_rank < best_rank ||
                (candidate_rank == best_rank &&
                    node->hostname < picked_node->hostname)) {
                picked_node = node;
            }
        }
        return picked_node;
    }


    /**
     * @brief Select the node that has the best invocation plan
     * @param plans a map of plans
     * @return a compute node
     */
    std::shared_ptr<ServerlessComputeNode> EvictionAverseServerlessPlanSelectionPolicy::pickBestInvocationPlan(
        const std::map<std::shared_ptr<ServerlessComputeNode>, std::shared_ptr<ServerlessSchedulingDecisions>>&
        plans) {
        return pickBestPlan(plans);
    }

    /**
    * @brief Select the node that has the best layer-load plan
    * @param plans a map of plans
    * @return a compute node
    */
    std::shared_ptr<ServerlessComputeNode> EvictionAverseServerlessPlanSelectionPolicy::pickBestLayerLoadPlan(
        const std::map<std::shared_ptr<ServerlessComputeNode>, std::shared_ptr<ServerlessSchedulingDecisions>>&
        plans) {
        return pickBestPlan(plans);
    }

    /**
    * @brief Select the node that has the best layer-copy plan
    * @param plans a map of plans
    * @return a compute node
    */
    std::shared_ptr<ServerlessComputeNode> EvictionAverseServerlessPlanSelectionPolicy::pickBestLayerCopyPlan(
        const std::map<std::shared_ptr<ServerlessComputeNode>, std::shared_ptr<ServerlessSchedulingDecisions>>&
        plans) {
        return pickBestPlan(plans);
    }
}
