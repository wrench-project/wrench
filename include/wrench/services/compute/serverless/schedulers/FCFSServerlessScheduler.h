//
// Created by Victor Pagan on 4/22/25.
//

#ifndef FCFSSERVERLESSSCHEDULER_H
#define FCFSSERVERLESSSCHEDULER_H

#include <wrench/services/compute/serverless/ServerlessScheduler.h>

namespace wrench {
    /**
     * @brief A class that implements a First Come First Serve (FCFS) scheduler to use in a
     *        serverless compute service.
     */
    class FCFSServerlessScheduler : public ServerlessScheduler {
    public:
        FCFSServerlessScheduler() = default;

        ~FCFSServerlessScheduler() override = default;

        std::shared_ptr<SchedulingDecisions> schedule(
            const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
            const std::shared_ptr<ServerlessStateOfTheSystem>& state) override;

    private:
        void makeImageDecisions(std::shared_ptr<SchedulingDecisions>& decisions,
                                const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
                                const std::shared_ptr<ServerlessStateOfTheSystem>& state);

        void makeInvocationDecisions(std::shared_ptr<SchedulingDecisions>& decisions,
                                     const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
                                     const std::shared_ptr<ServerlessStateOfTheSystem>& state);
    };
} // namespace wrench

#endif //FCFSSERVERLESSSCHEDULER_H
