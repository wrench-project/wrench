#ifndef SERVERLESSNODEACTIVATIONPOLICY_H
#define SERVERLESSNODEACTIVATIONPOLICY_H

#include <map>
#include <memory>
#include <set>

#include <simgrid/forward.h>

namespace wrench {

    class Invocation;
    class ServerlessSchedulingState;
    class ServerlessComputeNode;

    /**
     * @brief An abstract class to implement a serverless node activation policy
     */
    class ServerlessNodeActivationPolicy {
    public:

        virtual ~ServerlessNodeActivationPolicy() = default;

        /**
        * @brief Select a number of compute nodes to activate
        * @param scheduling_state the scheduling state
        * @return a set of compute nodes
        */
        virtual unsigned long selectNumNodesToActivate(
                const std::shared_ptr<ServerlessSchedulingState>& scheduling_state) = 0;

};

}

#endif //SERVERLESSNODEACTIVATIONPOLICY_H
