#ifndef SERVERLESSCONTAINERPREWARMPOLICY_H
#define SERVERLESSCONTAINERPREWARMPOLICY_H

#include <set>
#include <memory>

#include <wrench/services/compute/serverless/schedulers/scheduling_policies/ServerlessSchedulingPolicy.h>

namespace wrench {
    class Invocation;
    class Function;
    class ServerlessSchedulingState;
    class ServerlessComputeNode;

    /**
     * @brief An abstract class to implement a serverless container prewarm policy
     */
    class ServerlessContainerPrewarmPolicy : public ServerlessSchedulingPolicy {
    public:

        ServerlessContainerPrewarmPolicy() = default;

        ~ServerlessContainerPrewarmPolicy() override = default;

        /**
         * @brief Method to pick containers to prewarm
         * @param scheduling_state the scheduling state
         * @return a set of ServerlessComputeNode / Function pairs
         */
        virtual std::vector<std::pair<std::shared_ptr<Function>, std::shared_ptr<ServerlessComputeNode>>> prewarmContainers(
            const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) = 0;

    };
}

#endif //SERVERLESSCONTAINERPREWARMPOLICY_H
