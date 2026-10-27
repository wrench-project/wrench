#include <vector>

#include <wrench/services/compute/serverless/schedulers/scheduling_policies/container_prewarm_policy/NeverPrewarmContainerPolicy.h>


namespace wrench {
    /**
    * @brief Method to pick containers to prewarm
    * @param scheduling_state the scheduling state
    * @return a set of ServerlessComputeNode / Function pairs
    */
    std::vector<std::pair<std::shared_ptr<Function>, std::shared_ptr<ServerlessComputeNode>>>
    NeverPrewarmContainerPolicy::prewarmContainers(
        const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) {
        // Do nothing
        return {};
    }
}
