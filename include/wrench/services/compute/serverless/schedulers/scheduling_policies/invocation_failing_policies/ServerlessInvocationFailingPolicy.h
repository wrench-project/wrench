#ifndef SERVERLESSINVOCATIONFAILINGPOLICY_H
#define SERVERLESSINVOCATIONFAILINGPOLICY_H

#include <set>
#include <memory>

#include <wrench/services/compute/serverless/schedulers/scheduling_policies/ServerlessSchedulingPolicy.h>

namespace wrench {
    class Invocation;
    class ServerlessSchedulingState;

    /**
     * @brief An abstract class to implement a serverless scheduling invocation failing policy
     */
    class ServerlessInvocationFailingPolicy : public ServerlessSchedulingPolicy {
    public:

        ServerlessInvocationFailingPolicy() = default;

        ~ServerlessInvocationFailingPolicy() override = default;

        /**
         * @brief Method to pick schedulable invocations to fail
         * @param scheduling_state the scheduling state
         * @return a set of invocations
         */
        virtual std::set<std::shared_ptr<Invocation>> failSchedulableInvocations(
            const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) = 0;

    };
}

#endif //SERVERLESSINVOCATIONFAILINGPOLICY_H
