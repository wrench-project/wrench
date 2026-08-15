#ifndef WRENCH_GREEDYSERVERLESSCHEDULER_H
#define WRENCH_GREEDYSERVERLESSCHEDULER_H

#include <wrench/services/compute/serverless/ServerlessScheduler.h>

#include "GreedySchedulingState.h"

namespace wrench {
    class GreedySchedulingState;
}

namespace wrench {
    /**
     * @brief A class that implements an abstract two-pass scheduler to use in a
     *        serverless compute service.
     */
    class GreedyServerlessScheduler : public ServerlessScheduler {
    public:
        GreedyServerlessScheduler() = default;

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        ~GreedyServerlessScheduler() override = default;

        std::shared_ptr<ServerlessSchedulingDecisions> schedule(
            const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
            const ServerlessStateOfTheSystem* state) override;

    protected:

        /**
         * @brief Sort schedulable invocations, where the first invocations are considered
         *        first when making scheduling decisions
         * @param scheduling_state the scheduling state
         * @param invocations the list of schedulable invocation
         * @return a sort list of invocations
         */
        virtual std::vector<std::shared_ptr<Invocation>> sortSchedulableInvocations(
            const std::shared_ptr<GreedySchedulingState>& scheduling_state,
            const std::vector<std::shared_ptr<Invocation>>& invocations) = 0;

        /**
         * @brief Given an invocation, pick a compute node
         * @param scheduling_state the scheduling state
         * @param invocation an invocation
         * @return a compute node (or nullptr if none)
         */
        virtual std::shared_ptr<ServerlessComputeNode> pickComputeNode(
            const std::shared_ptr<GreedySchedulingState>& scheduling_state,
            const std::shared_ptr<Invocation>& invocation) = 0;

        /***********************/
        /** \endcond          **/
        /***********************/
    };
} // namespace wrench

#endif //WRENCH_GREEDYSERVERLESSCHEDULER_H
