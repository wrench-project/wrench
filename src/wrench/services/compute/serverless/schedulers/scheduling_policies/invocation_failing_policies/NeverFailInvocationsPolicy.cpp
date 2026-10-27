#include <wrench/services/compute/serverless/schedulers/scheduling_policies/invocation_failing_policies/NeverFailInvocationsPolicy.h>

namespace wrench {

    /**
     * @brief Method to pick schedulable invocations to fail
     * @param scheduling_state the scheduling state
     * @return a set of invocations
     */
    std::set<std::shared_ptr<Invocation>> NeverFailInvocationsPolicy::failSchedulableInvocations(
        const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) {
        return {};
    }

};