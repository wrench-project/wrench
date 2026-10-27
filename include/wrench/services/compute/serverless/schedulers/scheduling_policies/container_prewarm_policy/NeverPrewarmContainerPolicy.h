#ifndef NEVERPREWARMSERVERLESSCONTAINERPREWARMPOLICY_H
#define NEVERPREWARMSERVERLESSCONTAINERPREWARMPOLICY_H

#include <set>
#include <memory>

#include <wrench/services/compute/serverless/schedulers/scheduling_policies/container_prewarm_policy/ServerlessContainerPrewarmPolicy.h>

namespace wrench {
    class Function;
    class ServerlessSchedulingState;
    class ServerlessComputeNode;

    /**
     * @brief A "never prewarm any container" serverless scheduling container prewarm policy
     */
    class NeverPrewarmContainerPolicy : public ServerlessContainerPrewarmPolicy {
    public:

        NeverPrewarmContainerPolicy() = default;

        ~NeverPrewarmContainerPolicy() override = default;

        std::vector<std::pair<std::shared_ptr<Function>, std::shared_ptr<ServerlessComputeNode>>> prewarmContainers(
            const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) override;

    };
}

#endif //NEVERPREWARMSERVERLESSCONTAINERPREWARMPOLICY_H
