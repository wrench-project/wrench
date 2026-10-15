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
     * @brief A serverless scheduling node activation policy that activates one node each time no free container slot is available
     */
    class ActivateOneWhenNoFreeSlotServerlessNodeActivationPolicy : public ServerlessNodeActivationPolicy {
    public:

        ~ActivateOneWhenNoFreeSlotServerlessNodeActivationPolicy() override = default;

        unsigned long selectNumNodesToActivate(
                const std::shared_ptr<ServerlessSchedulingState>& scheduling_state,
                const std::vector<std::shared_ptr<Invocation>>& invocations) override;

};

}

#endif //ACTIVATEONEWHENNOFREESLOTSERVERLESSNODEACTIVATIONPOLICY_H
