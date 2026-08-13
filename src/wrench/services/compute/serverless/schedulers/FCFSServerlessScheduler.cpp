#include <wrench.h>

#include <wrench/services/compute/serverless/schedulers/FCFSServerlessScheduler.h>
#include <wrench/logging/TerminalOutput.h>

WRENCH_LOG_CATEGORY(wrench_test_fcfs_scheduler, "Log category for FCFS serverless scheduler");

namespace wrench {

    /**
     * @brief Given the list of schedulable invocations and the current system state, decide:
     *   - which images to copy to compute nodes
     *   - which images to load into memory at compute nodes
     *   - which invocations to start at compute nodes
     *
     * @param schedulable_invocations A list of invocations whose images reside on the head node
     * @param state The current system state
     *
     * @return A SchedulingDecisions object
     */
    std::shared_ptr<ServerlessSchedulingDecisions> FCFSServerlessScheduler::schedule(
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
    void FCFSServerlessScheduler::makeImageDecisions(const std::shared_ptr<ServerlessSchedulingDecisions>& decisions,
                            const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
                            const ServerlessStateOfTheSystem* state) {
        // Copy data from the state of the system so we can simulate assignment
        auto available_cores = state->getAvailableCores();
        auto compute_nodes = state->getComputeNodes();

        // In a first phase we go through all the invocations in order, and while there is an
        // idle core on the compute nodes (going in order as well), we declare our intent to run that
        // invocation on the compute node.

        std::map<std::shared_ptr<ServerlessComputeNode>, std::set<std::shared_ptr<Image>>> required_images;

        // For each invocation, assign it to the first compute node with an available core
        for (const auto& invocation : schedulable_invocations) {
            auto image = invocation->getRegisteredFunction()->getImage();

            for (const auto& compute_node : compute_nodes) {
                auto num_available_cores = available_cores.at(compute_node);
                if (num_available_cores > 0) {
                    // Pick the first available node
                    // Decrement our own available core count for chosen node
                    available_cores[compute_node]--;

                    // Record that this node requires the image (avoiding duplicates by using a set)
                    required_images[compute_node].insert(image);

                    // Move to next invocation
                    break;
                }
            }
        }

        // For each compute node, determine the images to copy or load
        for (const auto& node : compute_nodes) {
            for (const auto& image_file : required_images[node]) {
                if (!state->isImageOnDiskAtNode(node, image_file) &&
                    !state->isImageBeingCopiedToNode(node, image_file)) {
                    decisions->image_copies_to_disk.push_back({image_file, node});
                }
                else if (state->isImageOnDiskAtNode(node, image_file) &&
                    !state->isImageInRAMAtNode(node, image_file) &&
                    !state->isImageBeingLoadedAtNode(node, image_file)) {
                    decisions->image_loads_to_RAM.push_back({image_file, node});
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
    void FCFSServerlessScheduler::makeInvocationDecisions(const std::shared_ptr<ServerlessSchedulingDecisions>& decisions,
                                 const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
                                 const ServerlessStateOfTheSystem* state) {

        // Take snapshots of the current resources available
        auto available_cores = state->getAvailableCores();
        auto available_disk = state->getAvailableDiskSpace();
        auto available_ram = state->getAvailableRAMSpace();

        std::set<std::shared_ptr<Container>> claimed_idle_containers;

        for (const auto& inv : schedulable_invocations) {

            // Get the image for this invocation
            auto image = inv->getRegisteredFunction()->getImage();

            // Try to run it on some compute node (this is stupid, as we don't pick a particular node, but this is FCFS)
            for (const auto& node : state->getComputeNodes()) {

                // If the image is not in RAM, forget it
                if (not state->isImageInRAMAtNode(node, image)) {
                    continue;
                }

                // First, see if there is an idle container we can re-use
                auto idling_container = node->findIdleContainer(inv->getRegisteredFunction().get(), claimed_idle_containers);
                if (idling_container) {
                    decisions->invocation_dispatches.push_back({inv, node.get(), idling_container});
                    claimed_idle_containers.insert(idling_container);
                    available_cores[node]--;
                    break;
                }

                // Then, see if there is a core on which we can start a new container, given the current RAM / Disk space
                auto num_available_cores = available_cores[node];
                if ((num_available_cores > 0) and
                    (available_disk[node] >= inv->getRegisteredFunction()->getDiskSpaceLimit()) and
                    (available_ram[node] >= inv->getRegisteredFunction()->getRAMSpaceLimit())) {
                    decisions->invocation_dispatches.push_back({inv, node.get(), nullptr});
                    available_cores[node]--;
                    available_disk[node] -= inv->getRegisteredFunction()->getDiskSpaceLimit();
                    available_ram[node] -= inv->getRegisteredFunction()->getRAMSpaceLimit();
                    break;
                }

                // Then, see if we can terminate idling containers for other images to free up space
                std::set<std::shared_ptr<Container>> to_terminate;
                bool possible = node->findIdleContainersToTerminate(inv->getRegisteredFunction()->getRAMSpaceLimit(), to_terminate);
                if (possible) {
                    for (const auto& victim : to_terminate) {
                        decisions->container_terminations.push_back({victim});
                        available_cores[node]++;
                        available_disk[node] += victim->getRegisteredFunction()->getDiskSpaceLimit();
                        available_ram[node] += victim->getRegisteredFunction()->getRAMSpaceLimit();
                    }
                    decisions->invocation_dispatches.push_back({inv, node.get(), nullptr});
                    available_cores[node]--;
                    available_disk[node] -= inv->getRegisteredFunction()->getDiskSpaceLimit();
                    available_ram[node] -= inv->getRegisteredFunction()->getRAMSpaceLimit();
                    break;
                }
            }
        }
    }
} // namespace wrench
