#ifndef WRENCH_RANDOM_SERVERLESS_SCHEDULER_H
#define WRENCH_RANDOM_SERVERLESS_SCHEDULER_H

#include <wrench.h>
#include <random>

#include "wrench/services/compute/serverless/schedulers/TwoPassServerlessScheduler.h"

namespace wrench {
    /**
     * @brief A class that implements a random scheduler to use in a
     *        serverless compute service.
     */
    class RandomServerlessScheduler : public TwoPassServerlessScheduler {
    public:
        RandomServerlessScheduler() : TwoPassServerlessScheduler() {
            _rng = std::mt19937(std::random_device{}());
        };

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

    protected:
        std::vector<std::shared_ptr<ServerlessComputeNode>> sortComputeNodesForInvocation(
            const ServerlessStateOfTheSystem* state,
            const std::shared_ptr<Invocation>& invocation) override;

        std::vector<std::shared_ptr<ServerlessComputeNode>> sortComputeNodes(
            const ServerlessStateOfTheSystem* state) override;

        std::vector<std::shared_ptr<Invocation>> sortSchedulableInvocations(
            const ServerlessStateOfTheSystem* state,
            const std::vector<std::shared_ptr<Invocation>>& invocations) override;

    private:
        std::mt19937 _rng;


        /***********************/
        /** \endcond          **/
        /***********************/
    };
}

#endif
