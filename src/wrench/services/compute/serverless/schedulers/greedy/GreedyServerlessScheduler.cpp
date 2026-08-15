#include <wrench.h>

#include <wrench/services/compute/serverless/schedulers/greedy/GreedyServerlessScheduler.h>
#include <wrench/logging/TerminalOutput.h>

#include "wrench/services/compute/serverless/schedulers/greedy/GreedySchedulingState.h"

WRENCH_LOG_CATEGORY(wrench_greedy_serverless_scheduler, "Log category for GreedyServerlessScheduler");

namespace wrench {
    /**
     * @brief Given the list of schedulable invocations and the current system state, decide on which
     *        compute node each invocation should go and then encode what needs to be done to make
     *        it happen.
     *
     * @param schedulable_invocations A list of schedulable (i.e., with image on the head node's disk) invocations
     * @param state the current system state
     *
     * @return A SchedulingDecisions object
     */
    std::shared_ptr<ServerlessSchedulingDecisions> GreedyServerlessScheduler::schedule(
        const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
        const ServerlessStateOfTheSystem* state) {
        auto decisions = std::make_shared<ServerlessSchedulingDecisions>();

        // Create a scheduling state
        auto scheduling_state = std::make_shared<GreedySchedulingState>(state, schedulable_invocations);

        // Sort the schedulable invocations
        auto sorted_schedulable_invocations = this->sortSchedulableInvocations(
            scheduling_state, schedulable_invocations);

        // Go through the invocations and pick target hosts
        for (const auto& inv : sorted_schedulable_invocations) {
            // Get the image for this invocation
            auto image = inv->getRegisteredFunction()->getImage();

            // Pick a target compute node
            auto target_node = this->pickComputeNode(scheduling_state, inv);
            if (!target_node) {
                continue;
            }

            /** Translate the decision into actionable items **/

            // Can we use an idle container to run the function?
            bool scheduled = false;
            if (scheduling_state->cores_available.at(target_node) > 0) {
                for (auto const& idle_container : scheduling_state->idle_containers.at(target_node)) {
                    if (idle_container->getRegisteredFunction() == inv->getRegisteredFunction().get()) {
                        // Encode the decision
                        decisions->invocation_dispatches.push_back({inv, target_node, idle_container});
                        // Update the scheduling state
                        scheduling_state->idle_containers.at(target_node).erase(idle_container);
                        scheduling_state->cores_available.at(target_node) -= 1;
                        scheduled = true;
                        break;
                    }
                }
            }
            if (scheduled) continue;

            // Can we start a new container to run the function because the image is in RAM?
            if ((scheduling_state->cores_available.at(target_node) > 0)  and
                (scheduling_state->images_in_ram.at(target_node).find(image) != scheduling_state->images_in_ram.at(target_node).end())) {
                    // Encode the decision
                    decisions->invocation_dispatches.push_back({inv, target_node, nullptr});
                    // Update the scheduling state
                    scheduling_state->cores_available.at(target_node) -= 1;
                    continue;
            }

            // If the image on its way to RAM, we'll schedule again later
            if (scheduling_state->images_on_their_way_to_ram.at(target_node).find(image) != scheduling_state->images_on_their_way_to_ram.at(target_node).end()) {
                continue;
            }

            // If the image is on disk initiate an image load, that will maybe work (if we're lucky with RAM space and LRU)
            if (scheduling_state->images_on_disk.at(target_node).find(image) != scheduling_state->images_on_disk.at(target_node).end()) {
                decisions->image_loads_to_RAM.push_back({image, target_node});
                // Update the scheduling state
                scheduling_state->images_on_their_way_to_ram.at(target_node).insert(image);
                continue;
            }

            // If the image is on its way to disk, we'll schedule again later
            if (scheduling_state->images_on_their_way_to_disk.at(target_node).find(image) != scheduling_state->images_on_their_way_to_disk.at(target_node).end()) {
                continue;
            }

            // Schedule an image copy, that will maybe work (if we're lucky with disk space and LRU)
            decisions->image_copies_to_disk.push_back({image, target_node});

        }
        return decisions;
    }


} // namespace wrench
