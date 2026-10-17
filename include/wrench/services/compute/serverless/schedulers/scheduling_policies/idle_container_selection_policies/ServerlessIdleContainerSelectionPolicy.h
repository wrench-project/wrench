#ifndef SERVERLESSIDLECONTAINERSELECTIONPOLICY_H
#define SERVERLESSIDLECONTAINERSELECTIONPOLICY_H

#include <set>
#include <memory>

#include <wrench/services/compute/serverless/schedulers/scheduling_policies/ServerlessSchedulingPolicy.h>

namespace wrench {
    class Invocation;
    class Container;
    class ServerlessComputeNode;
    class ServerlessSchedulingState;

    /**
     * @brief An abstract class to implement a serverless idle container selection policy
     */
    class ServerlessIdleContainerSelectionPolicy : public ServerlessSchedulingPolicy {
    public:

        ServerlessIdleContainerSelectionPolicy() = default;

        ~ServerlessIdleContainerSelectionPolicy() override = default;

        /**
         * @brief Method to selection an idle container to (re)use
         * @param scheduling_state the scheduling state
         * @param invocation the invocation to execute
         * @param candidate_nodes the nodes that can be considered
         * @return a ServerlessComputeNode/Container pair
         */
        virtual std::pair<std::shared_ptr<ServerlessComputeNode>, std::shared_ptr<Container>> selectIdleContainer(
            const std::shared_ptr<ServerlessSchedulingState>& scheduling_state,
            const std::shared_ptr<Invocation>& invocation,
            const std::vector<std::shared_ptr<ServerlessComputeNode>>& candidate_nodes) = 0;

    };
}

#endif //SERVERLESSIDLECONTAINERSELECTIONPOLICY_H
