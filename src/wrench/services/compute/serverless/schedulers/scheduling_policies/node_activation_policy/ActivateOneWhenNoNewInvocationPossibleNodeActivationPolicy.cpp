#include <wrench/services/compute/serverless/schedulers/scheduling_policies/node_activation_policies/ActivateOneWhenNoNewInvocationPossibleNodeActivationPolicy.h>
#include <wrench/services/compute/serverless/schedulers/ServerlessSchedulingState.h>
#include <wrench/services/compute/serverless/ServerlessComputeNode.h>

namespace wrench {
    /**
    * @brief Select a number of nodes to activate
    * @param scheduling_state the scheduling state
    * @return a set of compute nodes
    */
    unsigned long ActivateOneWhenNoNewInvocationPossibleNodeActivationPolicy::selectNumNodesToActivate(
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
            // If we're note at the per node invocation limit and a container can have one more invocation, we're good
            if (not activated_node->isAtPerNodeInvocationLimit() and not activated_node->areAllContainersAtInvocationLimit()) {
                return 0;
            }
            // If we're note at the per node invocation limit and a new container could be started, we're good
            if (not activated_node->isAtPerNodeInvocationLimit() and not activated_node->isAtPerNodeContainerLimit()) {
                return 0;
            }
        }
        return 1;
    }
}
