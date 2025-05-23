
#ifndef FCFSSERVERLESSSCHEDULER_H
#define FCFSSERVERLESSSCHEDULER_H

#include <wrench/services/compute/serverless/ServerlessScheduler.h>

namespace wrench {
    /**
     * @brief A class that implements a First Come First Serve (FCFS) scheduler to use in a
     *        serverless compute service. This is likely not representative of any real-world
     *        serverless deployment.
     */
    class FCFSServerlessScheduler : public ServerlessScheduler {
    public:
        FCFSServerlessScheduler() = default;

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        ~FCFSServerlessScheduler() override = default;

        std::shared_ptr<ServerlessSchedulingDecisions> schedule(
            const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
            const std::shared_ptr<ServerlessStateOfTheSystem>& state) override;

    private:
        void makeImageDecisions(const std::shared_ptr<ServerlessSchedulingDecisions>& decisions,
                                const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
                                const std::shared_ptr<ServerlessStateOfTheSystem>& state);

        void makeInvocationDecisions(const std::shared_ptr<ServerlessSchedulingDecisions>& decisions,
                                     const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
                                     const std::shared_ptr<ServerlessStateOfTheSystem>& state);

        /***********************/
        /** \endcond          **/
        /***********************/
    };
} // namespace wrench

#endif //FCFSSERVERLESSSCHEDULER_H
