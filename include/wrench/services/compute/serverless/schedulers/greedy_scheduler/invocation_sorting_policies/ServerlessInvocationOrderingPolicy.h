#ifndef SERVERLESSINVOCATIONSORTINGPOLICY_H
#define SERVERLESSINVOCATIONSORTINGPOLICY_H

#include <vector>
#include <memory>

namespace wrench {
    class Invocation;
    class ServerlessSchedulingState;
    class FCFSServerlessInvocationOrderingPolicy;

    /**
     * @brief An abstract class to implement a serverless invocation ordering policy
     */
    class ServerlessInvocationOrderingPolicy {
    public:

        ServerlessInvocationOrderingPolicy() = default;

        virtual ~ServerlessInvocationOrderingPolicy() = default;

        /**
         * @brief Method to sort the schedulable invocations
         * @param scheduling_state the scheduling state
         * @param invocations the invocations
         * @return a sorted list of invocations
         */
        virtual std::vector<std::shared_ptr<Invocation>> sortSchedulableInvocations(
            const std::shared_ptr<ServerlessSchedulingState>& scheduling_state,
            const std::vector<std::shared_ptr<Invocation>>& invocations) = 0;

    };
}

#endif //SERVERLESSINVOCATIONSORTINGPOLICY_H
