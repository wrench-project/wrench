/**
 * WorkloadBalancingServerlessScheduler.cpp
 */

#include "wrench/services/compute/serverless/schedulers/WorkloadBalancingServerlessScheduler.h"

namespace wrench {
    /**
     * @brief Given the list of schedulable invocations and the current system state, decide:
     *   - which images to copy to compute nodes
     *   - which images to load into memory at compute nodes
     *   - which invocations to start at compute nodes
     *
     * @param schedulable_invocations A list of invocations whose images reside on the head node
     * @param state The current system state
     * @return A SchedulingDecisions object
     */
    std::shared_ptr<ServerlessSchedulingDecisions> WorkloadBalancingServerlessScheduler::schedule(
        const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
        const ServerlessStateOfTheSystem* state) {
        auto decisions = std::make_shared<ServerlessSchedulingDecisions>();
        makeImageDecisions(decisions, schedulable_invocations, state);
        makeInvocationDecisions(decisions, schedulable_invocations, state);
        return decisions;
    }

    /**
     * @brief Helper method to make image decisions
     * @param decisions An object that contains scheduling decisions
     * @param schedulable_invocations A list of invocations whose images reside on the head node
     * @param state The current system state
     */
    void WorkloadBalancingServerlessScheduler::makeImageDecisions(
        const std::shared_ptr<ServerlessSchedulingDecisions>& decisions,
        const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
        const ServerlessStateOfTheSystem* state) {
        calculateFunctionWorkloads(schedulable_invocations);
        createAllocationPlan(state);

        for (const auto& [node, function_allocation] : allocation_plan) {
            std::set<std::shared_ptr<RegisteredFunction>> required_functions;

            // figure out which functions we need here
            for (const auto& [function , core_count] : function_allocation) {
                if (core_count > 0) {
                    required_functions.insert(function);
                }
            }

            // only copy each image if it's neither already on the node nor currently being copied
            for (const auto& function : required_functions) {
                // auto image = function_images[function];
                auto image = function->getImage();
                if (!state->isImageOnDiskAtNode(node, image)
                    && !state->isImageBeingCopiedToNode(node, image)) {
                    decisions->image_copies_to_disk.push_back(CopyImage{image, node});
                }
                else if (state->isImageOnDiskAtNode(node, image) &&
                    !state->isImageBeingLoadedAtNode(node, image) &&
                    !state->isImageInRAMAtNode(node, image)) {
                    decisions->image_loads_to_RAM.push_back(LoadImage{image, node});

                }
            }
        }
    }

    /**
     * @brief Helper method to make invocation decisions
    * @param decisions An object that contains scheduling decisions
     * @param schedulable_invocations A list of invocations whose images reside on the head node
     * @param state The current system state
     */
    void WorkloadBalancingServerlessScheduler::makeInvocationDecisions(
        const std::shared_ptr<ServerlessSchedulingDecisions>& decisions,
        const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
        const ServerlessStateOfTheSystem* state) {
        // Get current available cores
        auto availableCores = state->getAvailableCores();

        // Group invocations by function name
        std::unordered_map<std::shared_ptr<RegisteredFunction>, std::vector<std::shared_ptr<Invocation>>> invocations_by_function;
        for (const auto& inv : schedulable_invocations) {
            invocations_by_function[inv->getRegisteredFunction()].push_back(inv);
        }

        std::set<std::shared_ptr<Container>> claimed_idle_containers;

        // For each function in our allocation plan
        for (const auto& [node, function_allocation] : allocation_plan) {
            for (const auto& [function_name, cores_allocated] : function_allocation) {
                if (cores_allocated == 0 || invocations_by_function.find(function_name) == invocations_by_function.
                    end()) {
                    continue;
                }

                // Get invocations for this function
                auto& invocations = invocations_by_function[function_name];

                // Schedule up to cores_allocated invocations of this function to this node
                unsigned int scheduled = 0;
                while (scheduled < cores_allocated && !invocations.empty() && availableCores[node] > 0) {
                    auto inv = invocations.back();
                    invocations.pop_back();

                    // Make sure the image is on this node
                    auto image = inv->getRegisteredFunction()->getImage();
                    if (state->isImageInRAMAtNode(node, image)) {
                        auto idling_container = node->findIdleContainer(inv->getRegisteredFunction().get(), claimed_idle_containers);
                        if (idling_container and (claimed_idle_containers.find(idling_container) == claimed_idle_containers.end())) {
                            decisions->invocation_dispatches.push_back({inv, node.get(), idling_container});
                            claimed_idle_containers.insert(idling_container);
                        } else {
                            decisions->invocation_dispatches.push_back({inv, node.get(), nullptr});
                        }
                        availableCores[node]--;
                        scheduled++;
                    }
                }
            }
        }
    }


    /**
     * @brief Helper method
     * @param invocations A list of invocations
     */
    void WorkloadBalancingServerlessScheduler::calculateFunctionWorkloads(
        const std::vector<std::shared_ptr<Invocation>>& invocations) {
        // Clear existing data
        function_workloads.clear();
        function_pending_count.clear();

        // Process each invocation
        for (const auto& inv : invocations) {
            // Get time limit (we use this as runtime)
            const double time_limit = inv->getRegisteredFunction()->getTimeLimit();

            // Add to total workload
            function_workloads[inv->getRegisteredFunction()] += time_limit;

            // Increment count
            function_pending_count[inv->getRegisteredFunction()]++;
        }
    }

    /**
     * @brief Helper method
     * @param state Curent state
     */
    void WorkloadBalancingServerlessScheduler::createAllocationPlan(
        const ServerlessStateOfTheSystem* state) {
        // Clear existing plan
        allocation_plan.clear();

        // Get available cores on each node
        auto availableCores = state->getAvailableCores();

        // Calculate total cores and total workload
        unsigned total_cores = 0;
        double total_workload = 0.0;

        for (const auto& [node, cores] : availableCores) {
            total_cores += cores;
            allocation_plan[node] = {}; // Initialize empty allocation map for node
        }

        for (const auto& [function_name, workload] : function_workloads) {
            total_workload += workload;
        }

        if (total_workload == 0.0) {
            return; // No work to do
        }

        // Allocate cores proportionally to workload
        std::vector<std::pair<std::shared_ptr<RegisteredFunction>, unsigned>> function_core_allocation;

        for (const auto& [function_name, workload] : function_workloads) {
            const double proportion = workload / total_workload;
            auto cores_for_function = static_cast<unsigned>(std::ceil(proportion * total_cores));

            // Don't allocate more cores than pending invocations
            cores_for_function = std::min(cores_for_function,
                                          static_cast<unsigned>(function_pending_count[function_name]));

            if (cores_for_function > 0) {
                function_core_allocation.emplace_back(function_name, cores_for_function);
            }
        }

        // Sort functions by cores needed (descending)
        std::sort(function_core_allocation.begin(), function_core_allocation.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });

        // Distribute cores across nodes to minimize makespan with greedy bin-packing approach
        for (const auto& [function_name, cores_needed] : function_core_allocation) {
            unsigned cores_remaining = cores_needed;

            while (cores_remaining > 0) {
                // Find node with most available cores
                std::shared_ptr<ServerlessComputeNode> best_node = nullptr;
                unsigned best_available = 0;

                for (const auto& [node, cores] : availableCores) {
                    unsigned allocated = 0;
                    for (const auto& [_, count] : allocation_plan[node]) {
                        allocated += count;
                    }

                    unsigned available = cores > allocated ? cores - allocated : 0;

                    if (available > best_available) {
                        best_node = node;
                        best_available = available;
                    }
                }

                if (best_node == nullptr || best_available == 0) {
                    break; // No more space
                }

                // Allocate cores
                const unsigned to_allocate = std::min(cores_remaining, best_available);
                allocation_plan[best_node][function_name] += to_allocate;
                cores_remaining -= to_allocate;
            }
        }
    }
} // namespace wrench
