#include <wrench/services/compute/serverless/schedulers/scheduling_policies/node_activation_policies/ReactiveScaleUpScaleDownNodeActivationPolicy.h>
#include <wrench/services/compute/serverless/schedulers/ServerlessSchedulingState.h>
#include <wrench/services/compute/serverless/ServerlessComputeNode.h>

namespace wrench {
    /**
    * @brief Select a number of nodes to activate
    * @param scheduling_state the scheduling state
    * @return a set of compute nodes
    */
    std::pair<unsigned long, std::set<std::shared_ptr<ServerlessComputeNode>>>  ReactiveScaleUpScaleDownNodeActivationPolicy::makeNodeScalingPlan(
        const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) {

        std::set<std::shared_ptr<ServerlessComputeNode>> none = {};

        bool should_activate_one_node = true;

        // If no node to activate, nevermind
        if (scheduling_state->_num_deactivated_compute_nodes == 0) {
            should_activate_one_node = false;
        } else if (scheduling_state->_num_being_activated_compute_nodes > 0) {
            should_activate_one_node = false;
        } else {
            // Activate one node only if all limits on current nodes are reached
            for (auto const& activated_node : scheduling_state->_activated_compute_nodes) {
                // If we're not at the per node invocation limit and a container can have one more invocation, we're good
                if (not activated_node->isAtPerNodeInvocationLimit() and not activated_node->areAllContainersAtInvocationLimit()) {
                    should_activate_one_node = false;
                    break;
                }
                // If we're not at the per node invocation limit and a new container could be started, we're good
                if (not activated_node->isAtPerNodeInvocationLimit() and not activated_node->isAtPerNodeContainerLimit()) {
                    should_activate_one_node = false;
                    break;
                }
            }
        }

        std::shared_ptr<ServerlessComputeNode> node_to_deactivate = nullptr;
        if (not scheduling_state->_schedulable_invocations.empty()) {
            // nevermind
        } else {
            for (auto const& node : scheduling_state->_activated_compute_nodes) {
                if (not scheduling_state->_image_layers_on_their_way_to_disk.at(node).empty()) {
                    continue;
                }
                if (not scheduling_state->_image_layers_on_their_way_to_ram.at(node).empty()) {
                    continue;
                }
                if (scheduling_state->_initializing_containers.at(node).empty()) {
                    continue;
                }
                bool all_containers_idle = false;
                for (auto const &container : scheduling_state->_initialized_containers.at(node)) {
                    if (scheduling_state->_num_in_flight_invocations_on_container.at(container) != 0) {
                        all_containers_idle = true;
                        break;
                    }
                }
                if (not all_containers_idle) {
                    continue;
                }
                node_to_deactivate = node;
                break;
            }
        }

        if (should_activate_one_node and (node_to_deactivate != nullptr)) {
            throw std::runtime_error("ReactiveScaleUpScaleDownNodeActivationPolicy(): should not activate and de-active at the same time!");
        }

        if (should_activate_one_node) {
            return {1, none};
        } else {
            return {0, {node_to_deactivate}};
        }

    }
}
