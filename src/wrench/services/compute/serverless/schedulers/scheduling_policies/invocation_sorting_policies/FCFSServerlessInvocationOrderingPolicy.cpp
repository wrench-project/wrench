#include <random>
#include <algorithm>
#include <wrench/services/compute/serverless/schedulers/ServerlessSchedulingState.h>
#include <wrench/services/compute/serverless/schedulers/scheduling_policies/invocation_ordering_policies/FCFSServerlessInvocationOrderingPolicy.h>

#include "wrench/function/Invocation.h"

namespace wrench {
    /**
     * @brief Method to sort the schedulable invocations in the scheduling state
     * @param scheduling_state the scheduling state
     */
    void FCFSServerlessInvocationOrderingPolicy::sortSchedulableInvocations(
        const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) {
        std::sort(
            scheduling_state->_schedulable_invocations.begin(),
            scheduling_state->_schedulable_invocations.end(),
            [](const std::shared_ptr<Invocation>& a,
               const std::shared_ptr<Invocation>& b) {
                return std::make_tuple(a->getSubmitDate(), a->getId()) <
                    std::make_tuple(b->getSubmitDate(), b->getId());
            });
        return;
    }

}
