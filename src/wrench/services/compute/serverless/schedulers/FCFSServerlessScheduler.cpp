#include <wrench.h>

#include <wrench/services/compute/serverless/schedulers/FCFSServerlessScheduler.h>
#include <wrench/logging/TerminalOutput.h>

WRENCH_LOG_CATEGORY(wrench_test_fcfs_scheduler, "Log category for FCFS serverless scheduler");

namespace wrench {
    // In this method we simulate a FCFS assignment of invocations to compute nodes,
    // and then determine per-node which images need to be copied (if missing), loaded (if not loaded), or removed (a random extra image).
    std::shared_ptr<ImageManagementDecision> FCFSServerlessScheduler::manageImages(
        const std::vector<std::shared_ptr<Invocation>>& schedulableInvocations,
        const std::shared_ptr<ServerlessStateOfTheSystem>& state) {
        auto decision = std::make_shared<ImageManagementDecision>();

        // Copy data from the state of the system so we can simulate assignment
        auto available_cores = state->getAvailableCores();
        auto compute_nodes = state->getComputeHosts();

        // In a first phase we go through all the invocations in order, and while there is an
        // idle core on the compute nodes (going in order as well), we declare our intent to run that
        // invocation on the compute node.

        std::map<std::string, std::set<std::shared_ptr<DataFile>>> required_images;

        // For each invocation, assign it to the first compute node with an available core
        for (const auto& invocation : schedulableInvocations) {
            auto image_file = invocation->getRegisteredFunction()->getFunctionImage()->getFile();

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
                    decision->imagesToCopyToComputeNode[node].push_back(image_file);
                    }
                else if (state->isImageOnNode(node, image_file) &&
                    !state->isImageInRAMAtNode(node, image_file) &&
                    !state->isImageBeingLoadedAtNode(node, image_file)) {
                    decision->imagesToLoadIntoRAMAtComputeNode[node].push_back(image_file);
                    }
            }
        }

        return decision;
    }

    // Implementation of scheduleFunctions
    std::vector<std::pair<std::shared_ptr<Invocation>, std::string>> FCFSServerlessScheduler::scheduleFunctions(
        const std::vector<std::shared_ptr<Invocation>>& schedulableInvocations,
        const std::shared_ptr<ServerlessStateOfTheSystem>& state) {

        std::vector<std::pair<std::shared_ptr<Invocation>, std::string>> decisions;

        auto available_cores = state->getAvailableCores();

        for (const auto& inv : schedulableInvocations) {
            // Get the image for this invocation
            auto image_file = inv->getRegisteredFunction()->getFunctionImage()->getFile();

            for (const auto& [hostname, num_available_cores] : available_cores) {
                // Checking if the node has available cores and if the image is on the node
                if (num_available_cores > 0 && state->isImageOnNode(hostname, image_file) && state->isImageInRAMAtNode(hostname, image_file)) {
                    decisions.emplace_back(inv, hostname);
                    available_cores[hostname]--;
                    // Move on to next invocation
                    break;
                }
            }
        }

        return decisions;
    }

    // FCFSServerlessScheduler::~FCFSServerlessScheduler() = default;
} // namespace wrench
