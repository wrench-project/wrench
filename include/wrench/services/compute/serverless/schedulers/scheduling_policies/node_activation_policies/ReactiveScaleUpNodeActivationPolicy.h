#ifndef REACTIVESCALEUPSERVERLESSNODEACTIVATIONPOLICY_H
#define REACTIVESCALEUPSERVERLESSNODEACTIVATIONPOLICY_H

#include <map>
#include <memory>
#include <set>

#include <simgrid/forward.h>

#include <wrench/services/compute/serverless/schedulers/scheduling_policies/node_activation_policies/ServerlessNodeScalingPolicy.h>


namespace wrench {

    class Invocation;
    class ServerlessSchedulingState;
    class ServerlessComputeNode;

    /**
     * @brief A serverless scheduling node activation policy that activates one node each time no new invocation
     *        could be placed anywhere
     */
    class ReactiveScaleUpNodeActivationPolicy : public ServerlessNodeScalingPolicy {
    public:

        ~ReactiveScaleUpNodeActivationPolicy() override = default;

        std::pair<unsigned long, std::set<std::shared_ptr<ServerlessComputeNode>>> makeNodeScalingPlan(
            const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) override;

};

}

#endif //REACTIVESCALEUPSERVERLESSNODEACTIVATIONPOLICY_H
