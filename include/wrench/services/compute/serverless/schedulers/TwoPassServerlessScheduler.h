#ifndef WRENCH_TWOPASSSCHEDULER_H
#define WRENCH_TWOPASSSCHEDULER_H

#include <wrench/services/compute/serverless/ServerlessScheduler.h>

namespace wrench {
    /**
     * @brief A class that implements an abstract two-pass scheduler  to use in a
     *        serverless compute service.
     */
    class TwoPassServerlessScheduler : public ServerlessScheduler {
    public:
        TwoPassServerlessScheduler() = default;

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        ~TwoPassServerlessScheduler() override = default;

        std::shared_ptr<ServerlessSchedulingDecisions> schedule(
            const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
            const ServerlessStateOfTheSystem* state) override;

    protected:
        virtual std::vector<std::shared_ptr<ServerlessComputeNode>> sortComputeNodesForInvocation(
            const ServerlessStateOfTheSystem* state,
            const std::shared_ptr<Invocation>& invocation) = 0;

        virtual std::vector<std::shared_ptr<ServerlessComputeNode>> sortComputeNodes(
            const ServerlessStateOfTheSystem* state) = 0;

        virtual std::vector<std::shared_ptr<Invocation>> sortSchedulableInvocations(
            const ServerlessStateOfTheSystem* state,
            const std::vector<std::shared_ptr<Invocation>>& invocation) = 0;

    private:
        virtual void makeImageDecisions(const std::shared_ptr<ServerlessSchedulingDecisions>& decisions,
                                const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
                                const ServerlessStateOfTheSystem* state);

        virtual void makeInvocationDecisions(const std::shared_ptr<ServerlessSchedulingDecisions>& decisions,
                                     const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
                                     const ServerlessStateOfTheSystem* state);

        /***********************/
        /** \endcond          **/
        /***********************/
    };
} // namespace wrench

#endif //WRENCH_TWOPASSSCHEDULER_H
