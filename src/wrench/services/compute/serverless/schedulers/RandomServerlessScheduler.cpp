#include <wrench.h>

#include <wrench/services/compute/serverless/schedulers/RandomServerlessScheduler.h>
#include <wrench/logging/TerminalOutput.h>

WRENCH_LOG_CATEGORY(wrench_test_random_scheduler, "Log category for random serverless scheduler");

namespace wrench {
    // Constructor implementation
    RandomServerlessScheduler::RandomServerlessScheduler()
        : rng(std::random_device()()) { }

    // In this method we simulate a random assignment of invocations to compute nodes,
    // and then determine per-node which images need to be copied (if missing) or removed (a random extra image).
    std::shared_ptr<ImageManagementDecision> RandomServerlessScheduler::manageImages(
        const std::vector<std::shared_ptr<Invocation>>& schedulableInvocations,
        const std::shared_ptr<ServerlessStateOfTheSystem>& state
    ) {
        auto decision = std::make_shared<ImageManagementDecision>();
        
        // Copy available cores so we can simulate assignment
        auto availableCores = state->getAvailableCores();
        
        // Mapping: compute node -> set of required image IDs
        std::map<std::string, std::unordered_set<std::string>> requiredImages;
        // Mapping: compute node -> vector of required DataFile pointers
        std::map<std::string, std::vector<std::shared_ptr<DataFile>>> requiredDataFiles;

        // For each invocation, randomly assign it to a compute node that has an available core
        for (const auto& inv : schedulableInvocations) {
            auto imageFile = inv->getRegisteredFunction()->getFunctionImage();
            std::string imageID = imageFile->getID();

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
                requiredImages[chosenNode].insert(imageID);
                auto& vec = requiredDataFiles[chosenNode];
                // Avoid duplicates
                bool exists = false;
                for (const auto& df : vec) {
                    if (df->getID() == imageID) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    vec.push_back(imageFile);
                }
            }
            // If no node is available, this invocation is skipped for assignment
        }

        // For each compute node, determine the images to copy and to remove
        auto computeNodes = state->getComputeHosts();

        for (const auto& node : computeNodes) {
            // Get required images for this node
            std::unordered_set<std::string> reqIDs;
            if (requiredImages.find(node) != requiredImages.end()) {
                reqIDs = requiredImages[node];
            }
            
            // In manageImages, while iterating over each required image for a node:
            for (const auto& reqID : reqIDs) {
                for (const auto& df : requiredDataFiles[node]) {
                    if (df->getID() == reqID) {
                        // Schedule copying only if the image isn't on the node and isn't already being copied.
                        if (!state->isImageOnNode(node, df) && 
                            !state->isImageBeingCopiedToNode(node, df)) {
                            decision->imagesToCopy[node].push_back(df);
                        }
                        break;  // Exit inner loop upon finding the corresponding DataFile.
                    }
                }
            }


            // For removing images, get the images already copied to the node
            std::set<std::shared_ptr<DataFile>> currentFiles = state->getImagesCopiedToNode(node);
            std::vector<std::shared_ptr<DataFile>> extras;
            
            for (const auto& df : currentFiles) {
                if (reqIDs.find(df->getID()) == reqIDs.end()) {
                    extras.push_back(df);
                }
            }
            
            if (!extras.empty()) {
                std::uniform_int_distribution<size_t> dist(0, extras.size() - 1);
                const auto& chosenExtra = extras[dist(rng)];
                decision->imagesToRemove[node].push_back(chosenExtra);
            }
        }

        return decision;
    }

    // Implementation of scheduleFunctions
    std::vector<std::pair<std::shared_ptr<Invocation>, std::string>> RandomServerlessScheduler::scheduleFunctions(
        const std::vector<std::shared_ptr<Invocation>>& schedulableInvocations,
        const std::shared_ptr<ServerlessStateOfTheSystem>& state
    ) {
        std::vector<std::pair<std::shared_ptr<Invocation>, std::string>> schedulingDecisions;
        auto availableCores = state->getAvailableCores();
        // log the availble cores
        WRENCH_INFO("Available cores: ");
        for (const auto& entry : availableCores) {
            WRENCH_INFO("Node: %s, Cores: %lu", entry.first.c_str(), entry.second);
        }

        // For each invocation, build a list of candidate nodes and pick one at random
        for (const auto& inv : schedulableInvocations) {

            // check if inv has function input
            WRENCH_INFO("Function input for invocation: %p",
                inv->_function_input.get());
            WRENCH_INFO("FUNCTION INVOCATION NAME: %s",
                inv->getRegisteredFunction()->getFunction()->getName().c_str());
            // Get the image for this invocation
            auto imageFile = inv->getRegisteredFunction()->getFunctionImage();
            
            std::vector<std::string> candidates;
            for (const auto& entry : availableCores) {
                if (entry.second > 0) {
                    // Only consider nodes that have the image already copied
                    // log
                    WRENCH_INFO("Checking node %s for image %s", entry.first.c_str(), imageFile->getID().c_str());
                    if (state->isImageOnNode(entry.first, imageFile)) {
                        candidates.push_back(entry.first);
                    }
                }
            }
            
            if (!candidates.empty()) {
                std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
                std::string chosenNode = candidates[dist(rng)];
                schedulingDecisions.emplace_back(inv, chosenNode);
                WRENCH_INFO("Function input for choosen invocation: %p",
                    inv->_function_input.get());
                availableCores[chosenNode]--;
            } else {
                // No suitable node with the image available; this invocation will be 
                // scheduled in a future iteration after image copying completes
                WRENCH_INFO("No suitable node available for invocation [%s] with image [%s]",
                            inv->getRegisteredFunction()->getFunction()->getName().c_str(),
                            imageFile->getID().c_str());
            }
        }
        
        return schedulingDecisions;
    }

    // RandomServerlessScheduler::~RandomServerlessScheduler() = default;

} // namespace wrench
