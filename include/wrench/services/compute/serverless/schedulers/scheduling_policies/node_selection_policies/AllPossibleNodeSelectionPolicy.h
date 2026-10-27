

#ifndef SERVERLESSSELECTALLPOSSIBLENOTSELECTIONPOLICY_H
#define SERVERLESSSELECTALLPOSSIBLENOTSELECTIONPOLICY_H

#include <wrench/services/compute/serverless/schedulers/scheduling_policies/node_selection_policies/ServerlessNodeSelectionPolicy.h>

namespace wrench {

    class Invocation;
    class ServerlessSchedulingState;
    class ServerlessComputeNode;

    /**
     * @brief A class that implements a serverless scheduling node selection policy that returns all compute nodes on which an invocation could be executed
     */
    class AllPossibleNodeSelectionPolicy : public ServerlessNodeSelectionPolicy {

        public:

        ~AllPossibleNodeSelectionPolicy() override = default;

        std::set<std::shared_ptr<ServerlessComputeNode>> selectCandidateNodes(
                const std::shared_ptr<ServerlessSchedulingState>& scheduling_state,
                const std::shared_ptr<Invocation>& invocation) override;

    };

}

#endif //SERVERLESSSELECTALLPOSSIBLENOTSELECTIONPOLICY_H
