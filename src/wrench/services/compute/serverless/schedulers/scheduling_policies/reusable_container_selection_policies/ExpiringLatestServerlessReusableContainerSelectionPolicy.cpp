#include <memory>
#include <wrench/services/compute/serverless/schedulers/scheduling_policies/idle_container_selection_policies/ExpiringLatestServerlessReusableContainerSelectionPolicy.h>

#include "wrench/function/Invocation.h"
#include "wrench/services/compute/serverless/Container.h"
#include "wrench/services/compute/serverless/schedulers/ServerlessSchedulingState.h"

namespace wrench {
    /**
     * @brief Select a reusable container:
     *    - If there is a non-idle container, pick the one with the fewest remaining "slots"
     *    - Among full idle containers, pick the one that will expire the latest
     * @param scheduling_state the scheduling state
     * @param invocation the invocation to execute
     * @param candidate_nodes the nodes that can be considered
     * @return a ServerlessComputeNode/Container pair
     */
    std::pair<std::shared_ptr<ServerlessComputeNode>, std::shared_ptr<Container>> ExpiringLatestServerlessReusableContainerSelectionPolicy::selectReusableContainer(
        const std::shared_ptr<ServerlessSchedulingState>& scheduling_state, const std::shared_ptr<Invocation>& invocation,
        const std::vector<std::shared_ptr<ServerlessComputeNode>>& candidate_nodes) {
        std::shared_ptr<ServerlessComputeNode> picked_node = nullptr;
        std::shared_ptr<Container> picked_container = nullptr;
        double picked_expiration_date = 0;
        unsigned long picked_remaining_invocation_slots = 0;

        for (const auto& node: candidate_nodes) {
            for (auto const& container : scheduling_state->_initialized_containers.at(node)) {
                // Container for the correct function?
                if (container->getFunction() != invocation->getFunction().get()) {
                    continue;
                }
                // If the container is already fully planned, nevermind
                auto remaining_invocation_slots = scheduling_state->_num_possible_new_invocations_on_container.at(container);
                if (remaining_invocation_slots == 0) {
                    continue;
                }

                auto expiration_date = container->getExpirationDate();
                if (not picked_container or (remaining_invocation_slots < picked_remaining_invocation_slots) or
                    ((remaining_invocation_slots == picked_remaining_invocation_slots) and (expiration_date > picked_expiration_date)) or
                    ((remaining_invocation_slots == picked_remaining_invocation_slots) and (expiration_date == picked_expiration_date) and
                        container->getCreationId() < picked_container->getCreationId())) {
                    picked_container = container;
                    picked_node = node;
                    picked_expiration_date = expiration_date;
                    picked_remaining_invocation_slots = remaining_invocation_slots;
                }
            }
        }
        return {picked_node, picked_container};
    }
}
