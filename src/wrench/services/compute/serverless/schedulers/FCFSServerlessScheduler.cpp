#include <wrench.h>

#include <wrench/services/compute/serverless/schedulers/FCFSServerlessScheduler.h>
#include <wrench/logging/TerminalOutput.h>

WRENCH_LOG_CATEGORY(wrench_test_serverless_fcfs_scheduler, "Log category for FCFS serverless scheduler");

namespace wrench {
    /**
     * @brief Sort compute nodes, with the first compute nodes considered first
     *        when making scheduling decisions
     * @param state the state of the system
     * @return a sorted list of ServerlessComputeNodes
     */
    std::vector<std::shared_ptr<ServerlessComputeNode>> FCFSServerlessScheduler::sortComputeNodes(
        const ServerlessStateOfTheSystem* state) {
        // Do nothing (FCFS order is original compute node order)
        return state->getComputeNodes();
    }


    /**
     * @brief Sort compute nodes for an invocation, where the first compute nodes
     *        are considered first when making scheduling decisions for this invocation
     * @param state the state of the system
     * @param invocation an invocation
     * @return a sorted list of compute nodes
     */
    std::vector<std::shared_ptr<ServerlessComputeNode>> FCFSServerlessScheduler::sortComputeNodesForInvocation(
        const ServerlessStateOfTheSystem* state, const std::shared_ptr<Invocation>& invocation) {
        // Do nothing (FCFS order is original compute node order)
        return state->getComputeNodes();
    }


    /**
     * @brief Sort schedulable invocations, where the first invocations are considered
     *        first when making scheduling decisions
     * @param state the state of the system
     * @param invocations the list of schedulable invocation
     * @return a sort list of invocations
     */
    std::vector<std::shared_ptr<Invocation>> FCFSServerlessScheduler::sortSchedulableInvocations(
        const ServerlessStateOfTheSystem* state, const std::vector<std::shared_ptr<Invocation>>& invocations) {
        // Do nothing (FCFS order is original invocation order)
        return invocations;
    }
} // namespace wrench
