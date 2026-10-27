#include <algorithm>

#include <wrench/services/compute/serverless/schedulers/scheduling_policies/node_activation_policies/ReactiveScaleUpNodeActivationPolicy.h>
#include <wrench/services/compute/serverless/schedulers/ServerlessSchedulingState.h>
#include <wrench/services/compute/serverless/ServerlessComputeNode.h>
#include "wrench/services/compute/serverless/Container.h"

namespace wrench {
    /**
    * @brief Select a number of nodes to activate
    * @param scheduling_state the scheduling state
    * @return a set of compute nodes
    */
    std::pair<unsigned long, std::set<std::shared_ptr<ServerlessComputeNode>>>
    ReactiveScaleUpNodeActivationPolicy::makeNodeScalingPlan(
        const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) {
        std::set<std::shared_ptr<ServerlessComputeNode>> none = {};

        const bool has_pending =
           std::any_of(
               scheduling_state->_pending_invocation_counts.begin(),
               scheduling_state->_pending_invocation_counts.end(),
               [](const auto& entry) {
                   return entry.second != 0;
               });

        // If nothing is pending, nevermind
        if (not has_pending) {
            return std::make_pair(0, none);
        }

        // If no node to activate, nevermind
        if (scheduling_state->_num_deactivated_compute_nodes == 0) {
            return {0, none};
        }

        // If one activation in process, nevermind
        if (scheduling_state->_num_being_activated_compute_nodes > 0) {
            return {0, none};
        }

        // Activate one node only if at least one schedulable function invocation cannot run
        bool all_functions_could_run = true;
        for (auto const& [function, count] : scheduling_state->_pending_invocation_counts) {

            if (count == 0) {
                continue;
            }

            bool function_could_run = false;
            for (auto const& node : scheduling_state->_activated_compute_nodes) {
                // If no more invocations can run on the node then the function cannot run

                if (scheduling_state->_num_possible_new_invocations_on_compute_node.at(node) == 0) {
                    continue;
                }

                // Count how many possible schedulable invocations could be placed for each relevant function
                bool function_could_run_on_current_container = false;
                for (auto const& container : scheduling_state->_initialized_containers.at(node)) {
                    if (container->getFunction() != function) {
                        continue;
                    }
                    if (scheduling_state->_num_in_flight_invocations_on_container.at(container) == container->
                        getMaxNumInflightInvocations()) {
                        continue;
                    }
                    function_could_run_on_current_container = true;
                    break;
                }
                function_could_run = function_could_run_on_current_container or (scheduling_state->_num_possible_new_containers_on_compute_node.at(node) > 0);
                if (function_could_run) {
                    break;
                }
            }
            all_functions_could_run = all_functions_could_run && function_could_run;
            if (not all_functions_could_run) {
                break;
            }
        }
        if (not all_functions_could_run) {
            return {1, none};
        } else {
            return {0, none};
        }
    }
}
