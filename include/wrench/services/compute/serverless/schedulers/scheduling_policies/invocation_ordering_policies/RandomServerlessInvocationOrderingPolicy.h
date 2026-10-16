#ifndef RANDOMSERVERLESSINVOCATIONORDERINGPOLICY_H
#define RANDOMSERVERLESSINVOCATIONORDERINGPOLICY_H

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
    class RandomServerlessInvocationOrderingPolicy : public ServerlessInvocationOrderingPolicy {
    public:
        explicit RandomServerlessInvocationOrderingPolicy(unsigned int seed);
        ~RandomServerlessInvocationOrderingPolicy() override = default;

        void sortSchedulableInvocations(std::shared_ptr<ServerlessSchedulingState>& scheduling_state) override;

    private:
        std::mt19937 _rng;
    };
}

#endif //RANDOMSERVERLESSINVOCATIONORDERINGPOLICY_H
