#ifndef RANDOMSERVERLESSPLANSELECTIONPOLICY_H
#define RANDOMSERVERLESSPLANSELECTIONPOLICY_H

#include <vector>
#include <memory>
#include <random>
#include <set>

#include <wrench/services/compute/serverless/schedulers/greedy_scheduler/plan_selection_policies/ServerlessPlanSelectionPolicy.h>


namespace wrench {
    class Container;
    class ImageLayer;
    class ServerlessSchedulingState;
    class ServerlessComputeNode;
    class ServerlessSchedulingDecisions;
    class LRUServerlessInvocationOrderingPolicy;
    class FewestServerlessInvocationOrderingPolicy;

    /**
     * @brief A policy that selects the plan randomly
     */
    class RandomServerlessPlanSelectionPolicy : public ServerlessPlanSelectionPolicy {
    public:
        explicit RandomServerlessPlanSelectionPolicy(unsigned int seed);

        std::shared_ptr<ServerlessComputeNode> pickBestInvocationPlan(
            const std::map<std::shared_ptr<ServerlessComputeNode>,
                           std::shared_ptr<ServerlessSchedulingDecisions>>& plans) override;
        std::shared_ptr<ServerlessComputeNode> pickBestLayerLoadPlan(
            const std::map<std::shared_ptr<ServerlessComputeNode>,
                           std::shared_ptr<ServerlessSchedulingDecisions>>& plans) override;
        std::shared_ptr<ServerlessComputeNode> pickBestLayerCopyPlan(
            const std::map<std::shared_ptr<ServerlessComputeNode>,
                           std::shared_ptr<ServerlessSchedulingDecisions>>& plans) override;

    private:
        std::shared_ptr<ServerlessComputeNode> pickBestPlan(
            const std::map<std::shared_ptr<ServerlessComputeNode>,
                           std::shared_ptr<ServerlessSchedulingDecisions>>& plans);

        std::mt19937 _rng;
    };
}

#endif //RANDOMSERVERLESSPLANSELECTIONPOLICY_H
