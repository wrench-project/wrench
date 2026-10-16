#ifndef SERVERLESSSCHEDULINGPOLICY_H
#define SERVERLESSSCHEDULINGPOLICY_H

#include <cfloat>

namespace wrench {

    /**
     * @brief An abstract class to implement a serverless scheduling policy
     */
    class ServerlessSchedulingPolicy {
    public:
        ServerlessSchedulingPolicy() = default;;
        virtual ~ServerlessSchedulingPolicy() = default;

        /**
         * @brief Earliest absolute simulated date at which this policy requests another scheduling round
         *        (DBL_MAX: no future wake-up date, only even-based wake-ups)
         */
        [[nodiscard]]
        virtual double getNextWakeupDate() const {
            return DBL_MAX;
        }
    };
}

#endif //SERVERLESSSCHEDULINGPOLICY_H
