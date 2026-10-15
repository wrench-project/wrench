#ifndef FCFSSERVERLESSINVOCATIONORDERINGPOLICY_H
#define FCFSSERVERLESSINVOCATIONORDERINGPOLICY_H

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
    class FCFSServerlessInvocationOrderingPolicy : public ServerlessInvocationOrderingPolicy {
    public:

        FCFSServerlessInvocationOrderingPolicy() = default;

        ~FCFSServerlessInvocationOrderingPolicy() override = default;

        std::vector<std::shared_ptr<Invocation>> sortSchedulableInvocations(
            const std::shared_ptr<ServerlessSchedulingState>& scheduling_state,
            const std::vector<std::shared_ptr<Invocation>>& invocations) override;

    };
}

#endif //FCFSSERVERLESSINVOCATIONORDERINGPOLICY_H
