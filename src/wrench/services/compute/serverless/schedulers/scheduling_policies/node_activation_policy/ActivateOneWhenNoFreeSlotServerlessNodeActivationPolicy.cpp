#include <wrench/services/compute/serverless/schedulers/scheduling_policies/node_activation_policies/ActivateOneWhenNoFreeSlotServerlessNodeActivationPolicy.h>
#include <wrench/services/compute/serverless/schedulers/ServerlessSchedulingState.h>
#include <wrench/services/compute/serverless/ServerlessComputeNode.h>

namespace wrench {
    /**
    * @brief Select a number of nodes to activate
    * @param scheduling_state the scheduling state
    * @return a set of compute nodes
    */
    unsigned long ActivateOneWhenNoFreeSlotServerlessNodeActivationPolicy::selectNumNodesToActivate(
        const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) {

        // If no node to activate, nevermind
        if (scheduling_state->_num_deactivated_compute_nodes == 0) {
            return 0;
        }

        // If one activation in process, nevermind
        if (scheduling_state->_num_being_activated_compute_nodes > 0) {
            return 0;
        }

        // Activate one node only if all limits on current nodes are reached
        for (auto const& activated_node : scheduling_state->_activated_compute_nodes) {
            if (not activated_node->isAtPerNodeContainerLimit() or
                not activated_node->isAtPerNodeInvocationLimit() or
                not activated_node->areAllContainersAtInvocationLimit()) {
                return 0;
            }
        }
        return 1;
    }
}
