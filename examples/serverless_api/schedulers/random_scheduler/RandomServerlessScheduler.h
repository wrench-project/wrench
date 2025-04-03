#include <wrench.h>

#ifndef WRENCH_RANDOM_SERVERLESS_SCHEDULER_H
#define WRENCH_RANDOM_SERVERLESS_SCHEDULER_H

namespace wrench {
    class RandomServerlessScheduler : public ServerlessScheduler {
    public:
        RandomServerlessScheduler()
            : rng(std::random_device()()) { }
        ~RandomServerlessScheduler() override = default;

        std::shared_ptr<ImageManagementDecision> manageImages(
            const std::vector<std::shared_ptr<Invocation>>& schedulableInvocations,
            std::shared_ptr<ServerlessStateOfTheSystem> state
        ) override;

        std::vector<std::pair<std::shared_ptr<Invocation>, std::string>> scheduleFunctions(
            const std::vector<std::shared_ptr<Invocation>>& schedulableInvocations,
            std::shared_ptr<ServerlessStateOfTheSystem> state
        ) override;

    private:
        std::mt19937 rng;
    };
}

#endif