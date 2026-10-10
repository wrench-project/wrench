
#include <wrench/services/compute/serverless/schedulers/greedy_scheduler/node_selection_policies/SelectAllPossibleServerlessNodeSelectionPolicy.h>
#include <wrench/services/compute/serverless/schedulers/ServerlessSchedulingState.h>
namespace wrench {
        /**
        * @brief Select a set of candidate nodes for an invocation
        * @param scheduling_state the scheduling state
        * @param invocation the invocation to schedule
        * @return a set of compute nodes
        */
        std::set<std::shared_ptr<ServerlessComputeNode>> SelectAllPossibleServerlessNodeSelectionPolicy::selectCandidateNodes(
                const std::shared_ptr<ServerlessSchedulingState>& scheduling_state,
                const std::shared_ptr<Invocation>& invocation) {

                /* Just filter out all compute nodes that have no available slots */
                std::set<std::shared_ptr<ServerlessComputeNode>> not_fully_busy_compute_nodes;
                for (auto const& node : scheduling_state->_compute_nodes) {
                        if (scheduling_state->_slots_available.at(node) > 0) {
                                not_fully_busy_compute_nodes.insert(node);
                        }
                }
                return not_fully_busy_compute_nodes;
        }

}
