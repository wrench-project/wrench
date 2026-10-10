

#ifndef SELECTALLPOSSIBLESERVERLESSNOTSELECTIONPOLICY_H
#define SELECTALLPOSSIBLESERVERLESSNOTSELECTIONPOLICY_H

#include <wrench/services/compute/serverless/schedulers/greedy_scheduler/node_selection_policies/ServerlessNodeSelectionPolicy.h>

namespace wrench {

    class Invocation;
    class ServerlessSchedulingState;
    class ServerlessComputeNode;

    /**
     * @brief An abstract class to implement a serverless node evaluation policy
     */
    class SelectAllPossibleServerlessNodeSelectionPolicy : public ServerlessNodeSelectionPolicy {

        public:

        ~SelectAllPossibleServerlessNodeSelectionPolicy() override = default;

        std::set<std::shared_ptr<ServerlessComputeNode>> selectCandidateNodes(
                const std::shared_ptr<ServerlessSchedulingState>& scheduling_state,
                const std::shared_ptr<Invocation>& invocation) override;

    };

}

#endif //SELECTALLPOSSIBLESERVERLESSNOTSELECTIONPOLICY_H
