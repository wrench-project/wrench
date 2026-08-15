#include <wrench.h>

#include <wrench/services/compute/serverless/schedulers/greedy/FCFSServerlessScheduler.h>
#include "wrench/services/compute/serverless/schedulers/greedy/GreedySchedulingState.h"

#include <wrench/logging/TerminalOutput.h>
WRENCH_LOG_CATEGORY(wrench_test_serverless_fcfs_scheduler, "Log category for FCFS serverless scheduler");

namespace wrench {
    /**
     * @brief Sort schedulable invocations, where the first invocations are considered
     *        first when making scheduling decisions
     * @param scheduling_state the scheduling state
     * @param invocations the list of schedulable invocation
     * @return a sort list of invocations
     */
    std::vector<std::shared_ptr<Invocation>> FCFSServerlessScheduler::sortSchedulableInvocations(
        const std::shared_ptr<GreedySchedulingState>& scheduling_state,
        const std::vector<std::shared_ptr<Invocation>>& invocations) {
        // Just return the invocation in the submit order (i.e., do nothing)
        return invocations;
    }

    /**
     * @brief Given an invocation, pick a compute node
     * @param scheduling_state the scheduling state
     * @param invocation an invocation
     * @return a compute node (or nullptr if none)
     */
    std::shared_ptr<ServerlessComputeNode> FCFSServerlessScheduler::pickComputeNode(
        const std::shared_ptr<GreedySchedulingState>& scheduling_state,
        const std::shared_ptr<Invocation>& invocation) {
        auto needed_image = invocation->getRegisteredFunction()->getImage();

        /* Go through the compute node in multiple passes, each time lowering "standards" */

        // Pass #1: Idle core + Image on Disk + Image in RAM + Idle Container + Enough RAM + Enough Disk
        for (auto const& node : scheduling_state->compute_nodes) {
            if (scheduling_state->cores_available.at(node) < 1) {
                continue;
            }
            for (auto const& idle_container : scheduling_state->idle_containers.at(node)) {
                if (idle_container->getRegisteredFunction() == invocation->getRegisteredFunction().get()) {
                    return node;
                }
            }
        }

        // Pass #2: Idle core + Image on Disk + Image in RAM + Enough RAM + enough disk
        for (auto const &node : scheduling_state->compute_nodes) {
            if ((scheduling_state->cores_available[node] > 0) and
                (scheduling_state->images_in_ram[node].find(needed_image) != scheduling_state->images_in_ram[node].end())) {
                return node;
            }
        }

        // Pass #3: Idle core + Image on Disk + Image on way to RAM
        for (auto const& node : scheduling_state->compute_nodes) {
            if ((scheduling_state->cores_available[node] > 0) and
                (scheduling_state->images_on_their_way_to_ram[node].find(needed_image) != scheduling_state->
                    images_on_their_way_to_ram[node].end())) {
                return node;
            }
        }

        // Pass #4: Idle core + Image on Disk
        for (auto const& node : scheduling_state->compute_nodes) {
            if ((scheduling_state->cores_available[node] > 0) and
                (scheduling_state->images_on_disk[node].find(needed_image) != scheduling_state->images_on_disk[node].
                    end())) {
                return node;
            }
        }

        // Pass #5: Idle core + Image on way to Disk
        for (auto const& node : scheduling_state->compute_nodes) {
            if ((scheduling_state->cores_available[node] > 0) and
                (scheduling_state->images_on_their_way_to_disk[node].find(needed_image) != scheduling_state->
                    images_on_their_way_to_disk[node].end())) {
                return node;
            }
        }

        // Pass #6: Idle core (perhaps will get luck with LRU, etc.)
        for (auto const& node : scheduling_state->compute_nodes) {
            if (scheduling_state->cores_available[node] > 0) {
                return node;
            }
        }

        // Oh well
        return nullptr;
    }
} // namespace wrench
