#ifndef RANDOMSERVERLESSINVOCATIONSORTINGPOLICY_H
#define RANDOMSERVERLESSINVOCATIONSORTINGPOLICY_H

#include <vector>
#include <random>
#include <memory>

#include <wrench/services/compute/serverless/schedulers/greedy_scheduler/invocation_sorting_policies/ServerlessInvocationOrderingPolicy.h>

namespace wrench {

    class Invocation;
    class ServerlessSchedulingState;

    /**
     * @brief A policy that orders the (schedulable) invocations in random order
     */
    class RandomServerlessInvocationOrderingPolicy : public ServerlessInvocationOrderingPolicy {
    public:
        explicit RandomServerlessInvocationOrderingPolicy(unsigned int seed);
        ~RandomServerlessInvocationOrderingPolicy() override = default;

        std::vector<std::shared_ptr<Invocation>> sortSchedulableInvocations(
            const std::shared_ptr<ServerlessSchedulingState>& scheduling_state,
            const std::vector<std::shared_ptr<Invocation>>& invocations) override;

    private:
        std::mt19937 _rng;
    };
}

#endif //RANDOMSERVERLESSINVOCATIONSORTINGPOLICY_H
