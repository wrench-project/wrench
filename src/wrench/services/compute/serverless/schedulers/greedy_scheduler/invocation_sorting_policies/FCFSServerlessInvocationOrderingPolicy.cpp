#include <random>
#include <algorithm>
#include <wrench/services/compute/serverless/schedulers/greedy_scheduler/invocation_sorting_policies/FCFSServerlessInvocationOrderingPolicy.h>

#include "wrench/function/Invocation.h"

namespace wrench {
    /**
     * @brief Method to sort the schedulable invocations
     * @param scheduling_state the scheduling state
     * @param invocations the invocations
     * @return a sorted list of invocations
     */
    std::vector<std::shared_ptr<Invocation>> FCFSServerlessInvocationOrderingPolicy::sortSchedulableInvocations(
    const std::shared_ptr<ServerlessSchedulingState>& scheduling_state,
    const std::vector<std::shared_ptr<Invocation>>& invocations) {
        auto ordered = invocations;
        std::sort(
            ordered.begin(),
            ordered.end(),
            [](const std::shared_ptr<Invocation>& a,
               const std::shared_ptr<Invocation>& b) {
                return std::make_tuple(a->getSubmitDate(), a->getId()) <
                    std::make_tuple(b->getSubmitDate(), b->getId());
            });
        return ordered;
    }

}
