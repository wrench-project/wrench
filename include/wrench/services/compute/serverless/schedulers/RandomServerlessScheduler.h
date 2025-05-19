#ifndef WRENCH_RANDOM_SERVERLESS_SCHEDULER_H
#define WRENCH_RANDOM_SERVERLESS_SCHEDULER_H

#include <wrench.h>
#include <random>

#include "wrench/services/compute/serverless/ServerlessScheduler.h"

namespace wrench {

    /**
     * @brief A class that implements a random scheduler to use in a
     *        serverless compute service. Likely (hopefully?) not representative
     *        of any real-world serverless deployment.
     */
    class RandomServerlessScheduler : public ServerlessScheduler {
    public:

        RandomServerlessScheduler();

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        ~RandomServerlessScheduler() override = default;

        std::shared_ptr<SchedulingDecisions> schedule(
            const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
            const std::shared_ptr<ServerlessStateOfTheSystem>& state
        ) override;

    private:
        std::mt19937 rng;

        void makeImageDecisions(const std::shared_ptr<SchedulingDecisions>& decisions,
                                const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
                                const std::shared_ptr<ServerlessStateOfTheSystem>& state);

        void makeInvocationDecisions(const std::shared_ptr<SchedulingDecisions>& decisions,
                                     const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
                                     const std::shared_ptr<ServerlessStateOfTheSystem>& state);

        /***********************/
        /** \endcond          **/
        /***********************/
    };
}

#endif