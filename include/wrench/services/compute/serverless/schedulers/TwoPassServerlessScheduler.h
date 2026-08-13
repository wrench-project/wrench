#ifndef WRENCH_TWOPASSSCHEDULER_H
#define WRENCH_TWOPASSSCHEDULER_H

#include <wrench/services/compute/serverless/ServerlessScheduler.h>

namespace wrench {
    /**
     * @brief A class that implements an abstract two-pass scheduler to use in a
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
        /**
         * @brief Sort compute nodes, with the first compute nodes considered first
         *        when making scheduling decisions
         * @param state the state of the system
         * @return a sorted list of ServerlessComputeNodes
         */
        virtual std::vector<std::shared_ptr<ServerlessComputeNode>> sortComputeNodes(
            const ServerlessStateOfTheSystem* state) = 0;

        /**
         * @brief Sort schedulable invocations, where the first invocations are considered
         *        first when making scheduling decisions
         * @param state the state of the system
         * @param invocations the list of schedulable invocation
         * @return a sort list of invocations
         */
        virtual std::vector<std::shared_ptr<Invocation>> sortSchedulableInvocations(
            const ServerlessStateOfTheSystem* state,
            const std::vector<std::shared_ptr<Invocation>>& invocations) = 0;

        /**
         * @brief Given an invocation, sort compute nodes so that the first compute nodes
         *        are considered first when making scheduling decisions for this invocation
         * @param state the state of the system
         * @param invocation an invocation
         * @return a sorted list of compute nodes
         */
        virtual std::vector<std::shared_ptr<ServerlessComputeNode>> sortComputeNodesForInvocation(
            const ServerlessStateOfTheSystem* state,
            const std::shared_ptr<Invocation>& invocation) = 0;


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
