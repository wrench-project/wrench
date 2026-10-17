#ifndef EXPIRINGEARLIESTSERVERLESSIDLECONTAINERSELECTIONPOLICY_H
#define EXPIRINGEARLIESTSERVERLESSIDLECONTAINERSELECTIONPOLICY_H

#include <set>
#include <memory>

#include <wrench/services/compute/serverless/schedulers/scheduling_policies/idle_container_selection_policies/ServerlessIdleContainerSelectionPolicy.h>

namespace wrench {
    class Invocation;
    class ServerlessComputeNode;
    class Container;
    class ServerlessSchedulingState;

    /**
     * @brief An abstract class to implement a serverless idle container selection policy
     */
    class ExpiringLatestServerlessIdleContainerSelectionPolicy : public ServerlessIdleContainerSelectionPolicy {
    public:
        ExpiringLatestServerlessIdleContainerSelectionPolicy() = default;

        ~ExpiringLatestServerlessIdleContainerSelectionPolicy() override = default;

        std::pair<std::shared_ptr<ServerlessComputeNode>, std::shared_ptr<Container>> selectIdleContainer(
            const std::shared_ptr<ServerlessSchedulingState>& scheduling_state,
            const std::shared_ptr<Invocation>& invocation,
            const std::vector<std::shared_ptr<ServerlessComputeNode>>& candidate_nodes) override;
    };
}

#endif //EXPIRINGEARLIESTSERVERLESSIDLECONTAINERSELECTIONPOLICY_H
