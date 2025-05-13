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
    std::shared_ptr<SchedulingDecisions> RandomServerlessScheduler::schedule(
        const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
        const std::shared_ptr<ServerlessStateOfTheSystem>& state) {
        auto decision = std::make_shared<SchedulingDecisions>();

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
    void RandomServerlessScheduler::makeImageDecisions(std::shared_ptr<SchedulingDecisions>& decisions,
                                                       const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
                                                       const std::shared_ptr<ServerlessStateOfTheSystem>& state) {

        // Copy available cores so we can simulate assignment
        auto availableCores = state->getAvailableCores();
        auto available_ram = state->getAvailableRAM();


        // Mapping: compute node -> set of required image IDs
        // std::map<std::string, std::unordered_set<std::string>> requiredImages;
        // Mapping: compute node -> vector of required DataFile pointers
        std::map<std::string, std::set<std::shared_ptr<DataFile>>> requiredImages;

        // For each invocation, randomly assign it to a compute node that has an available core
        for (const auto& inv : schedulable_invocations) {
            auto imageFile = inv->getRegisteredFunction()->getOriginalImageLocation()->getFile();
            // std::string imageID = imageFile->getID();

            // Build list of nodes with available cores
            std::vector<std::string> candidates;
            for (const auto& entry : availableCores) {
                if (entry.second > 0) {
                    candidates.push_back(entry.first);
                }
            }

            if (!candidates.empty()) {
                // Pick a random candidate
                std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
                const std::string& chosenNode = candidates[dist(rng)];
                // Decrement available core for chosen node
                availableCores[chosenNode]--;

                // Record that this node requires the image
                requiredImages[chosenNode].insert(imageFile);
            }
            // If no node is available, this invocation is skipped for assignment
        }

        // For each compute node, determine the images to copy
        auto computeNodes = state->getComputeHosts();

        for (const auto& node : computeNodes) {
            // Get required images for this node
            auto requiredImageFiles = requiredImages[node];

            // In manageImages, while iterating over each required image for a node:
            for (const auto& df : requiredImageFiles) {
                // Schedule copying only if the image isn't on the node and isn't already being copied.
                if (!state->isImageOnNode(node, df) &&
                    !state->isImageBeingCopiedToNode(node, df)) {
                    decisions->images_to_copy_to_compute_node[node].push_back(df);
                }
                else if (state->isImageOnNode(node, df) &&
                    !state->isImageBeingLoadedAtNode(node, df) &&
                    !state->isImageInRAMAtNode(node, df) &&
                    available_ram[node] >= df->getSize()) {
                    decisions->images_to_load_into_RAM_at_compute_node[node].push_back(df);
                    available_ram[node] -= df->getSize();
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
    void RandomServerlessScheduler::makeInvocationDecisions(std::shared_ptr<SchedulingDecisions>& decisions,
                                const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
                                const std::shared_ptr<ServerlessStateOfTheSystem>& state) {

        auto availableCores = state->getAvailableCores();

        // For each invocation, build a list of candidate nodes and pick one at random
        for (const auto& inv : schedulable_invocations) {
            auto imageFile = inv->getRegisteredFunction()->getOriginalImageLocation()->getFile();

            std::vector<std::string> candidates;
            for (const auto& [hostname, num_available_cores
                ] : availableCores) {
                if (num_available_cores > 0) {
                    // Only consider nodes that have the image already in RAM
                    if (state->isImageInRAMAtNode(hostname, imageFile)) {
                        candidates.push_back(hostname);
                    }
                }
            }

            if (!candidates.empty()) {
                std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
                const std::string& chosen_node = candidates[dist(rng)];
                decisions->invocations_to_start_at_compute_node[chosen_node].push_back(inv);
                availableCores[chosen_node]--;
            }
            else {
                // No suitable node with the image available; this invocation will be 
                // scheduled in a future iteration after image copying completes
                // WRENCH_INFO("No suitable node available for invocation [%s] with image [%s]",
                //             inv->getRegisteredFunction()->getFunction()->getName().c_str(),
                //             imageFile->getID().c_str());
            }
        }
    }

} // namespace wrench
