#ifndef ACTIVATEALLSERVERLESSNODEACTIVATIONPOLICY_H
#define ACTIVATEALLSERVERLESSNODEACTIVATIONPOLICY_H

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
     * @brief A serverless scheduling node activation policy that activates all compute nodes
     */
    class ActivateAllNodePolicy : public ServerlessNodeScalingPolicy {
    public:

        ~ActivateAllNodePolicy() override = default;


        std::pair<unsigned long, std::set<std::shared_ptr<ServerlessComputeNode>>> makeNodeScalingPlan(
            const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) override;

};

}

#endif //ACTIVATEALLSERVERLESSNODEACTIVATIONPOLICY_H
