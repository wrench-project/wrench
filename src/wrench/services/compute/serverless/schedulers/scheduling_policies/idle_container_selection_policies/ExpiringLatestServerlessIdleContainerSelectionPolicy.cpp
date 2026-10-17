#include <memory>
#include <wrench/services/compute/serverless/schedulers/scheduling_policies/idle_container_selection_policies/ExpiringLatestServerlessIdleContainerSelectionPolicy.h>

#include "wrench/function/Invocation.h"
#include "wrench/services/compute/serverless/Container.h"
#include "wrench/services/compute/serverless/schedulers/ServerlessSchedulingState.h"

namespace wrench {
    /**
     * @brief Select the idle container, if any, that would expire the latest
     * @param scheduling_state the scheduling state
     * @param invocation the invocation to execute
     * @param candidate_nodes the nodes that can be considered
     * @return a ServerlessComputeNode/Container pair
     */
    std::pair<std::shared_ptr<ServerlessComputeNode>, std::shared_ptr<Container>> ExpiringLatestServerlessIdleContainerSelectionPolicy::selectIdleContainer(
        const std::shared_ptr<ServerlessSchedulingState>& scheduling_state, const std::shared_ptr<Invocation>& invocation,
        const std::vector<std::shared_ptr<ServerlessComputeNode>>& candidate_nodes) {
        std::shared_ptr<ServerlessComputeNode> picked_node = nullptr;
        std::shared_ptr<Container> picked_container = nullptr;
        double picked_expiration_date = 0;

        for (const auto& node: candidate_nodes) {
            for (auto const& idle_container : scheduling_state->_idle_containers.at(node)) {
                if (idle_container->getFunction() != invocation->getFunction().get()) {
                    continue;
                }
                if (picked_container == nullptr or picked_container->getExpirationDate() > picked_expiration_date) {
                    picked_container = idle_container;
                    picked_node = node;
                    picked_expiration_date = idle_container->getExpirationDate();
                }
            }
        }
        return {picked_node, picked_container};
    }
}
