#ifndef SERVERLESSNODESSCALINGPOLICY_H
#define SERVERLESSNODESSCALINGPOLICY_H

#include <map>
#include <utility>
#include <memory>
#include <set>

#include <simgrid/forward.h>

#include <wrench/services/compute/serverless/schedulers/scheduling_policies/ServerlessSchedulingPolicy.h>

namespace wrench {

    class Invocation;
    class ServerlessSchedulingState;
    class ServerlessComputeNode;

    /**
     * @brief An abstract class to implement a serverless scheduling node activation policy
     */
    class ServerlessNodeScalingPolicy : public ServerlessSchedulingPolicy {
    public:

        ~ServerlessNodeScalingPolicy() override = default;

        /**
        * @brief Decide on a node scaling plan, which consists of a number of compute nodes
        *        to activate (possibly zero), and a number of compute nodes to de-activate (possibly none).
        * @param scheduling_state the scheduling state
        * @return a pair of: 1) a number of compute nodes to activate; and 2) a set of compute nodes to de-activate.
        */
        virtual std::pair<unsigned long, std::set<std::shared_ptr<ServerlessComputeNode>>> makeNodeScalingPlan(
                const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) = 0;

};

}

#endif //SERVERLESSNODESSCALINGPOLICY_H
