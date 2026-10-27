#ifndef SCHEDULABLEQUEUESIZELIMITSERVERLESSINVOCATIONFAILINGPOLICY_H
#define SCHEDULABLEQUEUESIZELIMITSERVERLESSINVOCATIONFAILINGPOLICY_H

#include <set>
#include <memory>

#include <wrench/services/compute/serverless/schedulers/scheduling_policies/invocation_failing_policies/ServerlessInvocationFailingPolicy.h>

namespace wrench {
    class Invocation;
    class ServerlessSchedulingState;

    /**
     * @brief A serverless scheduling invocation failing policy that never fails any schedulable invocation
     */
    class QueueSizeLimitFailingPolicy : public ServerlessInvocationFailingPolicy {
    public:

        explicit QueueSizeLimitFailingPolicy(unsigned long queue_limit);

        ~QueueSizeLimitFailingPolicy() override = default;

        std::set<std::shared_ptr<Invocation>> failSchedulableInvocations(
            const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) override;

    private:
        unsigned long _queue_limit;
    };
}

#endif //SCHEDULABLEQUEUESIZELIMITSERVERLESSINVOCATIONFAILINGPOLICY_H
