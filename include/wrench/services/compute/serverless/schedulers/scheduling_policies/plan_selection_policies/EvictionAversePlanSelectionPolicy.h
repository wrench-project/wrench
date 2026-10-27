#ifndef SERVERLESSEVICTIONAVERSEPLANSELECTIONPOLICY_H
#define SERVERLESSEVICTIONAVERSEPLANSELECTIONPOLICY_H

#include <vector>
#include <memory>
#include <set>

#include <wrench/services/compute/serverless/schedulers/scheduling_policies/plan_selection_policies/ServerlessPlanSelectionPolicy.h>


namespace wrench {
    class Container;
    class ImageLayer;
    class ServerlessSchedulingState;
    class ServerlessComputeNode;
    class ServerlessSchedulingDecisions;
    class LRUServerlessInvocationOrderingPolicy;
    class FewestServerlessInvocationOrderingPolicy;

    /**
     * @brief A serverless scheduling plan selection policy that selects the plan that has, in lexicographic order:
     *          - The least number of idle container evictions
     *          - The least number of in-RAM layer evictions
     *          - The least number of on-disk layer evictions
     *          - The least number of to-RAM layer loads
     *          - The least number of to-disk layer copies
     */
    class EvictionAversePlanSelectionPolicy : public ServerlessPlanSelectionPolicy {
    public:
        EvictionAversePlanSelectionPolicy() = default;

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
    };
}

#endif //SERVERLESSEVICTIONAVERSEPLANSELECTIONPOLICY_H
