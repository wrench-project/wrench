#include <stdexcept>
#include <algorithm>

#include "wrench/services/compute/serverless/schedulers/scheduling_policies/node_activation_policies/ReactiveScaleUpScaleDownNodeActivationPolicy.h"
#include "wrench/services/compute/serverless/schedulers/ServerlessSchedulingState.h"
#include "wrench/services/compute/serverless/ServerlessComputeNode.h"
#include "wrench/services/compute/serverless/Container.h"


namespace wrench {
    /**
    * @brief Select a number of nodes to activate
    * @param scheduling_state the scheduling state
    * @return a set of compute nodes
    */
    std::pair<unsigned long, std::set<std::shared_ptr<ServerlessComputeNode>>>  ReactiveScaleUpScaleDownNodeActivationPolicy::makeNodeScalingPlan(
        const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) {

        std::set<std::shared_ptr<ServerlessComputeNode>> none = {};

        bool should_activate_one_node;

        // Activation
        const bool has_pending =
            std::any_of(
                scheduling_state->getPendingInvocationCounts().begin(),
                scheduling_state->getPendingInvocationCounts().end(),
                [](const auto& entry) {
                    return entry.second != 0;
                });

        if (not has_pending) {
            should_activate_one_node = false;
        } else if (scheduling_state->getNumDeactivatedComputeNodes() == 0) {
            should_activate_one_node = false;
        } else if (scheduling_state->getNumBeingActivatedComputeNodes() > 0) {
            should_activate_one_node = false;
        } else {
            bool all_functions_could_run = true;
            for (auto const& [function, count] : scheduling_state->getPendingInvocationCounts()) {

                if (count == 0) {
                    continue;
                }

                bool function_could_run = false;
                for (auto const& node : scheduling_state->getActivatedComputeNodes()) {
                    // If no more invocations can run on the node then the function cannot run

                    if (scheduling_state->getNumPossibleNewInvocationsAtComputeNode(node) == 0) {
                        continue;
                    }

                    // Count how many possible schedulable invocations could be placed for each relevant function
                    bool function_could_run_on_current_container = false;
                    for (auto const& container : scheduling_state->getInitializedContainers(node)) {
                        if (container->getFunction() != function) {
                            continue;
                        }
                        if (scheduling_state->getNumInFlightInvocationsOnContainer(container) == container->
                            getMaxNumInflightInvocations()) {
                            continue;
                            }
                        function_could_run_on_current_container = true;
                        break;
                    }
                    function_could_run = function_could_run_on_current_container or (scheduling_state->getNumPossibleNewContainersAtComputeNode(node) > 0);
                    if (function_could_run) {
                        break;
                    }
                }
                all_functions_could_run = all_functions_could_run && function_could_run;
                if (not all_functions_could_run) {
                    break;
                }
            }
            should_activate_one_node = not all_functions_could_run;
        }

        // Deactivations
        std::set<std::shared_ptr<ServerlessComputeNode>> to_deactivate = {};
        if (has_pending) {
            // nevermind
        } else {
            for (auto const& node : scheduling_state->getActivatedComputeNodes()) {
                if (not scheduling_state->getImageLayersOnTheirWayToDisk(node).empty()) {
                    continue;
                }
                if (not scheduling_state->getImageLayersOnTheirWayToRAM(node).empty()) {
                    continue;
                }
                if (not scheduling_state->getInitializingContainers(node).empty()) {
                    continue;
                }
                bool all_containers_idle = true;
                for (auto const &container : scheduling_state->getInitializedContainers(node)) {
                    if (scheduling_state->getNumInFlightInvocationsOnContainer(container) != 0) {
                        all_containers_idle = false;
                        break;
                    }
                }
                if (not all_containers_idle) {
                    continue;
                }
                to_deactivate.insert(node);
            }
        }

        if (should_activate_one_node and (not to_deactivate.empty())) {
            throw std::runtime_error("ReactiveScaleUpScaleDownNodeActivationPolicy(): should not activate and de-active at the same time!");
        }

        if (should_activate_one_node) {
            return {1, {}};
        }

        if (not to_deactivate.empty()) {
            return {0, to_deactivate};
        }

        return {0, {}};

    }
}
