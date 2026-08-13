#include <wrench.h>

#include <wrench/services/compute/serverless/schedulers/RandomServerlessScheduler.h>
#include <wrench/logging/TerminalOutput.h>

WRENCH_LOG_CATEGORY(wrench_test_serverless_random_scheduler, "Log category for Random serverless scheduler");

namespace wrench {
    /**
     * @brief Sort compute nodes, with the first compute nodes considered first
     *        when making scheduling decisions
     * @param state the state of the system
     * @return a sorted list of ServerlessComputeNodes
     */
    std::vector<std::shared_ptr<ServerlessComputeNode>> RandomServerlessScheduler::sortComputeNodes(
        const ServerlessStateOfTheSystem* state) {
        std::vector shuffled(state->getComputeNodes());
        std::shuffle(shuffled.begin(), shuffled.end(), _rng);
        return shuffled;
    }

    /**
     * @brief Sort schedulable invocations, where the first invocations are considered
     *        first when making scheduling decisions
     * @param state the state of the system
     * @param invocations the list of schedulable invocation
     * @return a sort list of invocations
     */
    std::vector<std::shared_ptr<Invocation>> RandomServerlessScheduler::sortSchedulableInvocations(
        const ServerlessStateOfTheSystem* state, const std::vector<std::shared_ptr<Invocation>>& invocations) {
        std::vector shuffled(invocations);
        std::shuffle(shuffled.begin(), shuffled.end(), _rng);
        return shuffled;
    }

    /**
     * @brief Given an invocation, sort compute nodes so that the first compute nodes
     *        are considered first when making scheduling decisions for this invocation
     * @param state the state of the system
     * @param invocation an invocation
     * @return a sorted list of compute nodes
     */
    std::vector<std::shared_ptr<ServerlessComputeNode>> RandomServerlessScheduler::sortComputeNodesForInvocation(
        const ServerlessStateOfTheSystem* state, const std::shared_ptr<Invocation>& invocation) {
        std::vector shuffled(state->getComputeNodes());
        std::shuffle(shuffled.begin(), shuffled.end(), _rng);
        return shuffled;
    }
} // namespace wrench
