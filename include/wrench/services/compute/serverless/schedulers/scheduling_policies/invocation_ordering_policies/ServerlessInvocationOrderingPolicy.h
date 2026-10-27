#ifndef SERVERLESSINVOCATIONORDERINGPOLICY_H
#define SERVERLESSINVOCATIONORDERINGPOLICY_H

#include <vector>
#include <memory>

#include <wrench/services/compute/serverless/schedulers/scheduling_policies/ServerlessSchedulingPolicy.h>

namespace wrench {
    class Invocation;
    class ServerlessSchedulingState;
    class FCFSInvocationOrderingPolicy;

    /**
     * @brief An abstract class to implement a serverless scheduling invocation ordering policy
     */
    class ServerlessInvocationOrderingPolicy : public ServerlessSchedulingPolicy {
    public:

        ServerlessInvocationOrderingPolicy() = default;

        ~ServerlessInvocationOrderingPolicy() override = default;

        /**
         * @brief Method to sort the schedulable invocations in the scheduling state
         * @param scheduling_state the scheduling state
         */
        virtual void sortSchedulableInvocations(const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) = 0;

    };
}

#endif //SERVERLESSINVOCATIONORDERINGPOLICY_H
