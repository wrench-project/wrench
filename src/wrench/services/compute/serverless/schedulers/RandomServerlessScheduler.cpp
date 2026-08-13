#include <wrench.h>

#include <wrench/services/compute/serverless/schedulers/RandomServerlessScheduler.h>
#include <wrench/logging/TerminalOutput.h>

WRENCH_LOG_CATEGORY(wrench_test_random_scheduler, "Log category for random serverless scheduler");

namespace wrench {
    // Constructor implementation
    RandomServerlessScheduler::RandomServerlessScheduler()
        : rng(std::random_device()()) {
    }

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
    std::shared_ptr<ServerlessSchedulingDecisions> RandomServerlessScheduler::schedule(
        const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
        const ServerlessStateOfTheSystem* state) {
        auto decision = std::make_shared<ServerlessSchedulingDecisions>();

        this->makeImageDecisions(decision, schedulable_invocations, state);
        this->makeInvocationDecisions(decision, schedulable_invocations, state);
        return decision;
    }

    /**
     * @brief Helper method to make image decisions
     * @param decisions An object that contains scheduling decisions
     * @param schedulable_invocations A list of invocations whose images reside on the head node
     * @param state The current system state
     */
    void RandomServerlessScheduler::makeImageDecisions(const std::shared_ptr<ServerlessSchedulingDecisions>& decisions,
                                                       const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
                                                       const ServerlessStateOfTheSystem* state) {

        // Copy available cores so we can simulate assignment
        auto availableCores = state->getAvailableCores();


        // Mapping: compute node -> vector of required DataFile pointers
        std::map<std::shared_ptr<ServerlessComputeNode>, std::set<std::shared_ptr<Image>>> requiredImages;

        // For each invocation, randomly assign it to a compute node that has an available core
        for (const auto& inv : schedulable_invocations) {
            auto image = inv->getRegisteredFunction()->getImage();
            // std::string imageID = imageFile->getID();

            // Build list of nodes with available cores
            std::vector<std::shared_ptr<ServerlessComputeNode>> candidates;
            for (const auto& [compute_node, num_cores] : availableCores) {
                if (num_cores > 0) {
                    candidates.push_back(compute_node);
                }
            }

            if (!candidates.empty()) {
                // Pick a random candidate
                std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
                const std::shared_ptr<ServerlessComputeNode>& chosenNode = candidates[dist(rng)];
                // Decrement available core for chosen node
                availableCores[chosenNode]--;

                // Record that this node requires the image
                requiredImages[chosenNode].insert(image);
            }
            // If no node is available, this invocation is skipped for assignment
        }

        // For each compute node, determine the images to copy
        auto computeNodes = state->getComputeNodes();

        for (const auto& node : computeNodes) {
            // Get required images for this node
            auto requiredImageFiles = requiredImages[node];

            // In manageImages, while iterating over each required image for a node:
            for (const auto& df : requiredImageFiles) {
                // Schedule copying only if the image isn't on the node and isn't already being copied.
                if (!state->isImageOnDiskAtNode(node, df) &&
                    !state->isImageBeingCopiedToNode(node, df)) {
                    decisions->image_copies_to_disk.push_back({df, node});
                }
                else if (state->isImageOnDiskAtNode(node, df) &&
                    !state->isImageBeingLoadedAtNode(node, df) &&
                    !state->isImageInRAMAtNode(node, df)) {
                    decisions->image_loads_to_RAM.push_back({df, node});
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
    void RandomServerlessScheduler::makeInvocationDecisions(const std::shared_ptr<ServerlessSchedulingDecisions>& decisions,
                                const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
                                const ServerlessStateOfTheSystem* state) {

        auto available_cores = state->getAvailableCores();
        auto available_disk = state->getAvailableDiskSpace();
        auto available_ram = state->getAvailableRAMSpace();

        std::set<std::shared_ptr<Container>> claimed_idle_containers;

        // For through the invocation in a shuffled order
        static std::mt19937 rng(std::random_device{}());
        std::vector<std::shared_ptr<Invocation>> shuffled_invocations(schedulable_invocations);
        std::shuffle(shuffled_invocations.begin(), shuffled_invocations.end(), rng);

        for (const auto& inv : shuffled_invocations) {
            auto image = inv->getRegisteredFunction()->getImage();

            // Go through the compute nodes in shuffled order
            std::vector<std::shared_ptr<ServerlessComputeNode>> compute_nodes(state->getComputeNodes());
            std::shuffle(compute_nodes.begin(), compute_nodes.end(), rng);

            for (auto const &node : compute_nodes) {

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
