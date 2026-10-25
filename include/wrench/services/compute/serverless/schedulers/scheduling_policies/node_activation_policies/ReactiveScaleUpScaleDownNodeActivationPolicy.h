#ifndef REACTIVESCALEUPSCALEDOWNSERVERLESSNODEACTIVATIONPOLICY_H
#define REACTIVESCALEUPSCALEDOWNSERVERLESSNODEACTIVATIONPOLICY_H

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
     *        could be placed anywhere, or de-actives all idle nodes whenever there are no schedulable invocations
     */
    class ReactiveScaleUpScaleDownNodeActivationPolicy : public ServerlessNodeScalingPolicy {
    public:

        ~ReactiveScaleUpScaleDownNodeActivationPolicy() override = default;

        std::pair<unsigned long, std::set<std::shared_ptr<ServerlessComputeNode>>> makeNodeScalingPlan(
            const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) override;

};

}

#endif //REACTIVESCALEUPSCALEDOWNSERVERLESSNODEACTIVATIONPOLICY_H
