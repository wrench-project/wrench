#ifndef WRENCH_RANDOM_SERVERLESS_SCHEDULER_H
#define WRENCH_RANDOM_SERVERLESS_SCHEDULER_H

#include <wrench.h>
#include <random>

#include "wrench/services/compute/serverless/ServerlessScheduler.h"

namespace wrench {
    class RandomServerlessScheduler : public ServerlessScheduler {
    public:

        RandomServerlessScheduler();
        ~RandomServerlessScheduler() override = default;

        std::shared_ptr<ImageManagementDecision> manageImages(
            const std::vector<std::shared_ptr<Invocation>>& schedulableInvocations,
            const std::shared_ptr<ServerlessStateOfTheSystem>& state
        ) override;

        std::vector<std::pair<std::shared_ptr<Invocation>, std::string>> scheduleFunctions(
            const std::vector<std::shared_ptr<Invocation>>& schedulableInvocations,
            const std::shared_ptr<ServerlessStateOfTheSystem>& state
        ) override;

    private:
        std::mt19937 rng;
    };
}

#endif