#ifndef EXPIRINGLATESTSERVERLESSIDLECONTAINERSELECTIONPOLICY_H
#define EXPIRINGLATESTSERVERLESSIDLECONTAINERSELECTIONPOLICY_H

#include <set>
#include <memory>

#include <wrench/services/compute/serverless/schedulers/scheduling_policies/idle_container_selection_policies/ServerlessIdleContainerSelectionPolicy.h>

namespace wrench {
    class Invocation;
    class ServerlessComputeNode;
    class Container;
    class ServerlessSchedulingState;

    /**
     * @brief A "whichever expires the latest" serverless scheduling reusable container selection policy
     *           - If there is a non-idle container, pick the one with the fewest remaining "slots"
     *           - Among full idle containers, pick the one that will expire the latest
     */
    class ExpiringLatestReusableContainerSelectionPolicy : public ServerlessReusableContainerSelectionPolicy {
    public:
        ExpiringLatestReusableContainerSelectionPolicy() = default;

        ~ExpiringLatestReusableContainerSelectionPolicy() override = default;

        std::pair<std::shared_ptr<ServerlessComputeNode>, std::shared_ptr<Container>> selectReusableContainer(
            const std::shared_ptr<ServerlessSchedulingState>& scheduling_state,
            const std::shared_ptr<Invocation>& invocation,
            const std::vector<std::shared_ptr<ServerlessComputeNode>>& candidate_nodes) override;
    };
}

#endif //EXPIRINGLATESTSERVERLESSIDLECONTAINERSELECTIONPOLICY_H
