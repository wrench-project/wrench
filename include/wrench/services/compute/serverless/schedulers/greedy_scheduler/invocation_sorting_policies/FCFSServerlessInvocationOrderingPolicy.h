#ifndef FCFSSERVERLESSINVOCATIONSORTINGPOLICY_H
#define FCFSSERVERLESSINVOCATIONSORTINGPOLICY_H

#include <vector>
#include <memory>

#include <wrench/services/compute/serverless/schedulers/greedy_scheduler/invocation_sorting_policies/ServerlessInvocationOrderingPolicy.h>

namespace wrench {

    class Invocation;
    class ServerlessSchedulingState;

    /**
     * @brief A policy that orders the (schedulable) invocations in
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

#endif //FCFSSERVERLESSINVOCATIONSORTINGPOLICY_H
