#include <wrench.h>

#include <wrench/services/compute/serverless/schedulers/greedy/RandomServerlessScheduler.h>
#include <wrench/logging/TerminalOutput.h>

WRENCH_LOG_CATEGORY(wrench_test_serverless_random_scheduler, "Log category for Random serverless scheduler");

namespace wrench {

    /**
     * @brief Sort schedulable invocations, where the first invocations are considered
     *        first when making scheduling decisions
     * @param scheduling_state the scheduling state
     * @param invocations the list of schedulable invocation
     * @return a sort list of invocations
     */
    std::vector<std::shared_ptr<Invocation>> RandomServerlessScheduler::sortSchedulableInvocations(
        const std::shared_ptr<GreedySchedulingState>& scheduling_state,
        const std::vector<std::shared_ptr<Invocation>>& invocations) {
        std::vector shuffled(invocations);
        std::shuffle(shuffled.begin(), shuffled.end(), _rng);
        return shuffled;
    }

    /**
     * @brief Given an invocation, pick a compute node
     * @param scheduling_state the scheduling state
     * @param invocation an invocation
     * @return a compute node (or nullptr if none)
     */
    std::shared_ptr<ServerlessComputeNode> RandomServerlessScheduler::pickComputeNode(
        const std::shared_ptr<GreedySchedulingState>& scheduling_state,
        const std::shared_ptr<Invocation>& invocation) {

        // Just return a random node
        std::uniform_int_distribution<size_t> dist(0, scheduling_state->compute_nodes.size() - 1);
        return scheduling_state->compute_nodes[dist(_rng)];
    }

} // namespace wrench
