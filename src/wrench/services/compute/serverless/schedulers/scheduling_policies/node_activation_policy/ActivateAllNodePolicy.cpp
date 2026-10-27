#include <wrench/services/compute/serverless/schedulers/scheduling_policies/node_activation_policies/ActivateAllNodePolicy.h>
#include <wrench/services/compute/serverless/schedulers/ServerlessSchedulingState.h>


namespace wrench {
    /**
    * @brief Select a set of candidate nodes for an invocation
    * @param scheduling_state the scheduling state
    * @return a set of compute nodes
    */
    std::pair<unsigned long, std::set<std::shared_ptr<ServerlessComputeNode>>> ActivateAllNodePolicy::makeNodeScalingPlan(
        const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) {
        std::set<std::shared_ptr<ServerlessComputeNode>> none = {};
        return {scheduling_state->_num_deactivated_compute_nodes, none};
    }
}
