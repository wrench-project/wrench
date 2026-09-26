#include <random>
#include <algorithm>
#include <wrench/services/compute/serverless/schedulers/greedy_scheduler/invocation_sorting_policies/RandomServerlessInvocationOrderingPolicy.h>

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
     * @brief Method to sort the schedulable invocations
     * @param scheduling_state the scheduling state
     * @param invocations the invocations
     * @return a sorted list of invocations
     */
    std::vector<std::shared_ptr<Invocation>> RandomServerlessInvocationOrderingPolicy::sortSchedulableInvocations(
    const std::shared_ptr<ServerlessSchedulingState>& scheduling_state,
    const std::vector<std::shared_ptr<Invocation>>& invocations) {
        std::vector shuffled(invocations);
        std::shuffle(shuffled.begin(), shuffled.end(), _rng);
        return shuffled;
    }

}
