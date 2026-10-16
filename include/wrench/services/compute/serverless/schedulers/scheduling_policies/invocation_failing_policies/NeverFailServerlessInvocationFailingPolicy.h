#ifndef NEVERFAILSERVERLESSINVOCATIONFAILINGPOLICY_H
#define NEVERFAILSERVERLESSINVOCATIONFAILINGPOLICY_H

#include <set>
#include <memory>

#include <wrench/services/compute/serverless/schedulers/scheduling_policies/invocation_failing_policies/ServerlessInvocationFailingPolicy.h>

namespace wrench {
    class Invocation;
    class ServerlessSchedulingState;

    /**
     * @brief A serverless scheduling invocation failing policy that never fails any schedulable invocation
     */
    class NeverFailServerlessInvocationFailingPolicy : public ServerlessInvocationFailingPolicy {
    public:

        NeverFailServerlessInvocationFailingPolicy() = default;

        ~NeverFailServerlessInvocationFailingPolicy() override = default;

        std::set<std::shared_ptr<Invocation>> failSchedulableInvocations(
            const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) override;
    };
}

#endif //NEVERFAILSERVERLESSINVOCATIONFAILINGPOLICY_H
