#ifndef ACTIVATEALLSERVERLESSNODEACTIVATIONPOLICY_H
#define ACTIVATEALLSERVERLESSNODEACTIVATIONPOLICY_H

#include <map>
#include <memory>
#include <set>

#include <simgrid/forward.h>

#include <wrench/services/compute/serverless/schedulers/greedy_scheduler/node_activation_policies/ServerlessNodeActivationPolicy.h>


namespace wrench {

    class Invocation;
    class ServerlessSchedulingState;
    class ServerlessComputeNode;

    /**
     * @brief A class to implement a serverless node activation policy
     */
    class ActivateAllServerlessNodeActivationPolicy : public ServerlessNodeActivationPolicy {
    public:

        ~ActivateAllServerlessNodeActivationPolicy() override = default;


        unsigned long selectNumNodesToActivate(
                const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) override;

};

}

#endif //ACTIVATEALLSERVERLESSNODEACTIVATIONPOLICY_H
