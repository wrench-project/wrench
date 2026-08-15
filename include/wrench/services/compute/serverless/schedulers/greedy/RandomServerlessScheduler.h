#ifndef WRENCH_RANDOM_SERVERLESS_SCHEDULER_H
#define WRENCH_RANDOM_SERVERLESS_SCHEDULER_H

#include <wrench.h>
#include <random>

#include "wrench/services/compute/serverless/schedulers/greedy/GreedyServerlessScheduler.h"

namespace wrench {
    /**
     * @brief A class that implements a random scheduler to use in a
     *        serverless compute service.
     */
    class RandomServerlessScheduler : public GreedyServerlessScheduler {

    public:
        /**
         * @brief Constructor
         * @param seed RNG seed
         */
        explicit RandomServerlessScheduler(const unsigned long seed) : GreedyServerlessScheduler() {
            // _rng = std::mt19937(std::random_device{}());
            _rng = std::mt19937(seed);
        };

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


    private:
        std::mt19937 _rng;


        /***********************/
        /** \endcond          **/
        /***********************/
    };
}

#endif
