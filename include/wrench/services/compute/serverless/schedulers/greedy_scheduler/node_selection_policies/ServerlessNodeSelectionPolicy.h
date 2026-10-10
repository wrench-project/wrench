#ifndef SERVERLESSNODESELECTIONPOLICY_H
#define SERVERLESSNODESELECTIONPOLICY_H

#include <map>
#include <memory>
#include <set>

#include <simgrid/forward.h>

namespace wrench {

    class Invocation;
    class ServerlessSchedulingState;
    class ServerlessComputeNode;

    /**
     * @brief An abstract class to implement a serverless node evaluation policy
     */
    class ServerlessNodeSelectionPolicy {
    public:

        virtual ~ServerlessNodeSelectionPolicy() = default;

        /**
        * @brief Select a set of candidate nodes for an invocation
        * @param scheduling_state the scheduling state
        * @param invocation the invocation to schedule
        * @return a set of compute nodes
        */
        virtual std::set<std::shared_ptr<ServerlessComputeNode>> selectCandidateNodes(
                const std::shared_ptr<ServerlessSchedulingState>& scheduling_state,
                const std::shared_ptr<Invocation>& invocation) = 0;

};

}

#endif //SERVERLESSNODESELECTIONPOLICY_H
