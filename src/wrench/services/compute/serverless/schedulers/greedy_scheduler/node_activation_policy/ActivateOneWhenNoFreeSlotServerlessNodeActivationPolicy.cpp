#include <wrench/services/compute/serverless/schedulers/greedy_scheduler/node_activation_policies/ActivateOneWhenNoFreeSlotServerlessNodeActivationPolicy.h>
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
        if (scheduling_state->_num_deactivated_compute_nodes == 0) {
            return 0;
        }

        for (auto const& activated_node : scheduling_state->_activated_compute_nodes) {
            if (activated_node->getNumFreeSlots() > 0) {
                return 0;
            }
        }
        return 1;
    }
}
