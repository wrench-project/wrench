#ifndef SERVERLESSFCFSINVOCATIONORDERINGPOLICY_H
#define SERVERLESSFCFSINVOCATIONORDERINGPOLICY_H

#include <vector>
#include <memory>

#include <wrench/services/compute/serverless/schedulers/scheduling_policies/invocation_ordering_policies/ServerlessInvocationOrderingPolicy.h>

namespace wrench {

    class Invocation;
    class ServerlessSchedulingState;

    /**
     * @brief A serverless scheduling invocation ordering policy that orders the (schedulable) invocations in
     *        FCFS (First Come First Serve) order
     */
    class FCFSInvocationOrderingPolicy : public ServerlessInvocationOrderingPolicy {
    public:

        FCFSInvocationOrderingPolicy() = default;

        ~FCFSInvocationOrderingPolicy() override = default;

        void sortSchedulableInvocations(const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) override;

    };
}

#endif //SERVERLESSFCFSINVOCATIONORDERINGPOLICY_H
