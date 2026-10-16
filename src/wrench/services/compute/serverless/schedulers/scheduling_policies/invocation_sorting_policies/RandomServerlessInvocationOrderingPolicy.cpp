#include <random>
#include <algorithm>
#include <wrench/services/compute/serverless/schedulers/ServerlessSchedulingState.h>
#include <wrench/services/compute/serverless/schedulers/scheduling_policies/invocation_ordering_policies/RandomServerlessInvocationOrderingPolicy.h>

#include "wrench/function/Invocation.h"

namespace wrench {

    /**
     * @brief Constructor
     * @param seed The seed of the RNG
     */
    RandomServerlessInvocationOrderingPolicy::RandomServerlessInvocationOrderingPolicy(const unsigned int seed) {
        _rng = std::mt19937(seed);
    }

    /**
     * @brief Method to sort the schedulable invocations in the scheduling state
     * @param scheduling_state the scheduling state
     * @return a sorted list of invocations
     */
    void RandomServerlessInvocationOrderingPolicy::sortSchedulableInvocations(
        const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) {
        std::shuffle(scheduling_state->_schedulable_invocations.begin(), scheduling_state->_schedulable_invocations.end(), _rng);
    }

}
