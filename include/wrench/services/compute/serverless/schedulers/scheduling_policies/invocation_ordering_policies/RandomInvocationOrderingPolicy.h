#ifndef SERVERLESSRANDOMINVOCATIONORDERINGPOLICY_H
#define SERVERLESSRANDOMINVOCATIONORDERINGPOLICY_H

#include <vector>
#include <random>
#include <memory>

#include <wrench/services/compute/serverless/schedulers/scheduling_policies/invocation_ordering_policies/ServerlessInvocationOrderingPolicy.h>

namespace wrench {

    class Invocation;
    class ServerlessSchedulingState;

    /**
     * @brief A serverless scheduling invocation ordering policy that orders the (schedulable) invocations in random order
     */
    class RandomInvocationOrderingPolicy : public ServerlessInvocationOrderingPolicy {
    public:
        explicit RandomInvocationOrderingPolicy(unsigned int seed);
        ~RandomInvocationOrderingPolicy() override = default;

        void sortSchedulableInvocations(const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) override;

    private:
        std::mt19937 _rng;
    };
}

#endif //SERVERLESSRANDOMINVOCATIONORDERINGPOLICY_H
