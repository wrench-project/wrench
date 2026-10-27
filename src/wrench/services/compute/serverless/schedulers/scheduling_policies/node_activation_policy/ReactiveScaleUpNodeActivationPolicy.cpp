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

        // If no node to activate, nevermind
        if (scheduling_state->_num_deactivated_compute_nodes == 0) {
            return {0, none};
        }

        // If one activation in process, nevermind
        if (scheduling_state->_num_being_activated_compute_nodes > 0) {
            return {0, none};
        }

        // Activate one node only if at least one schedulable function invocation cannot run
        bool could_all_functions_run = true;
        for (auto const& [function, inv] : scheduling_state->_outstanding_invocation_counts) {
            bool function_could_run = false;
            for (auto const& node : scheduling_state->_activated_compute_nodes) {
                // If no more invocations can run on the node then the function cannot run
                if (node->isAtPerNodeInvocationLimit()) {
                    continue;
                }

                // Count how many possible schedulable invocations could be placed for each relevant function
                bool function_could_run_on_current_container = false;
                for (auto const& container : scheduling_state->_initialized_containers.at(node)) {
                    if (container->getFunction() != function.get()) {
                        continue;
                    }
                    if (scheduling_state->_num_in_flight_invocations_on_container.at(container) == container->
                        getMaxNumInflightInvocations()) {
                        continue;
                    }
                    function_could_run_on_current_container = true;
                    break;
                }
                function_could_run = function_could_run_on_current_container or (not node->isAtPerNodeContainerLimit());
                if (function_could_run) {
                    break;
                }
            }
            could_all_functions_run = could_all_functions_run && function_could_run;
            if (not could_all_functions_run) {
                break;
            }
        }
        if (not could_all_functions_run) {
            return {1, none};
        } else {
            return {0, none};
        }
    }
}
