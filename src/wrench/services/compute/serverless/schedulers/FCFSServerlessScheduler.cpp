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
     * @return A SchedulingDecisions object
     */
    std::shared_ptr<SchedulingDecisions> FCFSServerlessScheduler::schedule(
        const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
        const std::shared_ptr<ServerlessStateOfTheSystem>& state) {
        auto decisions = std::make_shared<SchedulingDecisions>();
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
    void FCFSServerlessScheduler::makeImageDecisions(std::shared_ptr<SchedulingDecisions>& decisions,
                            const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
                            const std::shared_ptr<ServerlessStateOfTheSystem>& state) {
        // Copy data from the state of the system so we can simulate assignment
        auto available_cores = state->getAvailableCores();
        auto compute_nodes = state->getComputeHosts();

        // In a first phase we go through all the invocations in order, and while there is an
        // idle core on the compute nodes (going in order as well), we declare our intent to run that
        // invocation on the compute node.

        std::map<std::string, std::set<std::shared_ptr<DataFile>>> required_images;

        // For each invocation, assign it to the first compute node with an available core
        for (const auto& invocation : schedulable_invocations) {
            auto image_file = invocation->getRegisteredFunction()->getOriginalImageLocation()->getFile();

            for (const auto& [hostname, num_available_cores] : available_cores) {
                if (num_available_cores > 0) {
                    // Pick the first available node
                    // Decrement our own available core count for chosen node
                    available_cores[hostname]--;

                    // Record that this node requires the image (avoiding duplicates by using a set)
                    required_images[hostname].insert(image_file);

                    // Move to next invocation
                    break;
                }
            }
        }

        // For each compute node, determine the images to copy or load
        for (const auto& node : compute_nodes) {
            for (const auto& image_file : required_images[node]) {
                if (!state->isImageOnNode(node, image_file) &&
                    !state->isImageBeingCopiedToNode(node, image_file)) {
                    decisions->images_to_copy_to_compute_node[node].push_back(image_file);
                }
                else if (state->isImageOnNode(node, image_file) &&
                    !state->isImageInRAMAtNode(node, image_file) &&
                    !state->isImageBeingLoadedAtNode(node, image_file)) {
                    decisions->images_to_load_into_RAM_at_compute_node[node].push_back(image_file);
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
    void FCFSServerlessScheduler::makeInvocationDecisions(std::shared_ptr<SchedulingDecisions>& decisions,
                                 const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
                                 const std::shared_ptr<ServerlessStateOfTheSystem>& state) {
        auto available_cores = state->getAvailableCores();

        for (const auto& inv : schedulable_invocations) {

            // Get the image for this invocation
            auto image_file = inv->getRegisteredFunction()->getOriginalImageLocation()->getFile();

            for (const auto& [hostname, num_available_cores] : available_cores) {
                // Checking if the node has available cores and if the image is on the node
                if (num_available_cores > 0 && state->isImageInRAMAtNode(hostname, image_file)) {
                    decisions->invocations_to_start_at_compute_node[hostname].push_back(inv);
                    available_cores[hostname]--;
                    // Move on to next invocation
                    break;
                }
            }
        }
    }
} // namespace wrench
