#ifndef WRENCH_FCFSSERVERLESSSCHEDULER_H
#define WRENCH_FCFSSERVERLESSSCHEDULER_H

#include <wrench/services/compute/serverless/schedulers/greedy/GreedyServerlessScheduler.h>

#include "GreedyServerlessScheduler.h"

namespace wrench {

    class GreedySchedulingState;

    /**
     * @brief A class that implements a First-Come-First-Serve (FCFS) scheduler to use in a
     *        serverless compute service.
     */
    class FCFSServerlessScheduler : public GreedyServerlessScheduler {

    public:
        FCFSServerlessScheduler() : GreedyServerlessScheduler() {};

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

    protected:

        std::vector<std::shared_ptr<Invocation>> sortSchedulableInvocations(
            const std::shared_ptr<GreedySchedulingState>& scheduling_state,
            const std::vector<std::shared_ptr<Invocation>>& invocations) override;

        std::shared_ptr<ServerlessComputeNode> pickComputeNode(
            const std::shared_ptr<GreedySchedulingState>& scheduling_state,
            const std::shared_ptr<Invocation>& invocation) override;


        /***********************/
        /** \endcond          **/
        /***********************/
    };
} // namespace wrench

#endif //WRENCH_FCFSSERVERLESSSCHEDULER_H
