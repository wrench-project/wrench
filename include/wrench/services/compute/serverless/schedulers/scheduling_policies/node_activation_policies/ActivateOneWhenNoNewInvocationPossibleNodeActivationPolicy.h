#ifndef ACTIVATEONEWHENNOFREESLOTSERVERLESSNODEACTIVATIONPOLICY_H
#define ACTIVATEONEWHENNOFREESLOTSERVERLESSNODEACTIVATIONPOLICY_H

#include <map>
#include <memory>
#include <set>

#include <simgrid/forward.h>

#include <wrench/services/compute/serverless/schedulers/scheduling_policies/node_activation_policies/ServerlessNodeActivationPolicy.h>


namespace wrench {

    class Invocation;
    class ServerlessSchedulingState;
    class ServerlessComputeNode;

    /**
     * @brief A serverless scheduling node activation policy that activates one node each time no new invocation
     *        could be placed anywhere
     */
    class ActivateOneWhenNoNewInvocationPossibleNodeActivationPolicy : public ServerlessNodeActivationPolicy {
    public:

        ~ActivateOneWhenNoNewInvocationPossibleNodeActivationPolicy() override = default;

        unsigned long selectNumNodesToActivate(const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) override;

};

}

#endif //ACTIVATEONEWHENNOFREESLOTSERVERLESSNODEACTIVATIONPOLICY_H
