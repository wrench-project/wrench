#ifndef WRENCH_FCFSSERVERLESSSCHEDULER_H
#define WRENCH_FCFSSERVERLESSSCHEDULER_H

#include <wrench/services/compute/serverless/schedulers/TwoPassServerlessScheduler.h>

namespace wrench {
    /**
     * @brief A class that implements a First-Come-First-Serve (FCFS) scheduler to use in a
     *        serverless compute service.
     */
    class FCFSServerlessScheduler : public TwoPassServerlessScheduler {
    public:
        FCFSServerlessScheduler() : TwoPassServerlessScheduler() {};

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

    protected:

        std::vector<std::shared_ptr<ServerlessComputeNode>> sortComputeNodes(
            const ServerlessStateOfTheSystem* state) override;

        std::vector<std::shared_ptr<Invocation>> sortSchedulableInvocations(
            const ServerlessStateOfTheSystem* state,
            const std::vector<std::shared_ptr<Invocation>>& invocations) override;

        std::vector<std::shared_ptr<ServerlessComputeNode>> sortComputeNodesForInvocation(
            const ServerlessStateOfTheSystem* state,
            const std::shared_ptr<Invocation>& invocation) override;

        /***********************/
        /** \endcond          **/
        /***********************/
    };
} // namespace wrench

#endif //WRENCH_FCFSSERVERLESSSCHEDULER_H
