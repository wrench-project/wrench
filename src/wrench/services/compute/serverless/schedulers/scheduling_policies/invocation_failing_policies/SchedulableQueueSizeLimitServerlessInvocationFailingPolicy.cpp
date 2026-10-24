
#include <stdexcept>

#include <wrench/services/compute/serverless/schedulers/scheduling_policies/invocation_failing_policies/SchedulableQueueSizeLimitServerlessInvocationFailingPolicy.h>
#include "wrench/services/compute/serverless/schedulers/ServerlessSchedulingState.h"

namespace wrench {
    /**
     * @brief Constructor
     * @param queue_limit The schedulable queue size limit (>1, up to ULONG_MAX)
     */
    SchedulableQueueSizeLimitServerlessInvocationFailingPolicy::
    SchedulableQueueSizeLimitServerlessInvocationFailingPolicy(unsigned long queue_limit) :
        ServerlessInvocationFailingPolicy() {
        if (queue_limit < 1) {
            throw std::invalid_argument(
                "SchedulableQueueSizeLimitServerlessInvocationFailingPolicy(): queue_limit must be positive");
        }
        _queue_limit = queue_limit;
    }

    /**
     * @brief Method to pick schedulable invocations to fail
     * @param scheduling_state the scheduling state
     * @return a set of invocations
     */
    std::set<std::shared_ptr<Invocation>>
    SchedulableQueueSizeLimitServerlessInvocationFailingPolicy::failSchedulableInvocations(
        const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) {
        std::size_t num_to_fail = (_queue_limit < scheduling_state->_schedulable_invocations.size()
                                       ? scheduling_state->_schedulable_invocations.size() - _queue_limit
                                       : 0);

        if (num_to_fail == 0) {
            return {};
        }

        std::set<std::shared_ptr<Invocation>> to_fail;
        auto iterator_offset = static_cast<std::vector<std::shared_ptr<wrench::Invocation>>::difference_type>(num_to_fail);
        for (auto it = scheduling_state->_schedulable_invocations.end() - iterator_offset;
             it != scheduling_state->_schedulable_invocations.end(); ++it) {
            to_fail.insert(*it);
        }
        return to_fail;
    }
};
