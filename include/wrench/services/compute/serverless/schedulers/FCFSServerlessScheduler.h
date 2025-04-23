//
// Created by Victor Pagan on 4/22/25.
//

#ifndef FCFSSERVERLESSSCHEDULER_H
#define FCFSSERVERLESSSCHEDULER_H

#include <wrench/services/compute/serverless/ServerlessScheduler.h>

namespace wrench {
    class FCFSServerlessScheduler : public ServerlessScheduler {
    public:
        FCFSServerlessScheduler() = default;

        ~FCFSServerlessScheduler() override = default;

        std::shared_ptr<ImageManagementDecision> manageImages(
            const std::vector<std::shared_ptr<Invocation> > &schedulableInvocations,
            const std::shared_ptr<ServerlessStateOfTheSystem> &state
        ) override;

        std::vector<std::pair<std::shared_ptr<Invocation>, std::string> > scheduleFunctions(
            const std::vector<std::shared_ptr<Invocation> > &schedulableInvocations,
            const std::shared_ptr<ServerlessStateOfTheSystem> &state
        ) override;
    };
} // namespace wrench

#endif //FCFSSERVERLESSSCHEDULER_H
