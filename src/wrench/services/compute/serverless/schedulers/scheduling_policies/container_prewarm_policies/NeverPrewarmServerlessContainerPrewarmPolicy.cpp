#include <wrench/services/compute/serverless/schedulers/scheduling_policies/container_prewarm_policy/NeverPrewarmServerlessContainerPrewarmPolicy.h>


namespace wrench {
    /**
    * @brief Method to pick containers to prewarm
    * @param scheduling_state the scheduling state
    * @param invocations the invocations
    * @return a set of ServerlessComputeNode / Function pairs
    */
    std::set<std::pair<std::shared_ptr<Function>, std::shared_ptr<ServerlessComputeNode>>>
    NeverPrewarmServerlessContainerPrewarmPolicy::prewarmContainers(
        const std::shared_ptr<ServerlessSchedulingState>& scheduling_state,
        const std::vector<std::shared_ptr<Invocation>>& invocations) {
        // Do nothing
        return {};
    }
}
