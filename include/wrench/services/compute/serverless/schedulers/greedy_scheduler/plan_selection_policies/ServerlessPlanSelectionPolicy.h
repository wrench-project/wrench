#ifndef SERVERLESSPLANSELECTIONPOLICY_H
#define SERVERLESSPLANSELECTIONPOLICY_H

#include <map>
#include <memory>
#include <set>

#include <simgrid/forward.h>

namespace wrench {

    class Container;
    class ImageLayer;
    class ServerlessSchedulingState;
    class ServerlessComputeNode;
    class ServerlessSchedulingDecisions;
    class LRUServerlessInvocationOrderingPolicy;
    class FewestServerlessInvocationOrderingPolicy;

    /**
     * @brief An abstract class to implement a serverless plan evaluation policy
     */
    class ServerlessPlanSelectionPolicy {
    public:

        virtual ~ServerlessPlanSelectionPolicy() = default;

        /**
        * @brief Select the node that has the best invocation plan
        * @param plans a map of plans
        * @return a compute node
        */
        virtual std::shared_ptr<ServerlessComputeNode> pickBestInvocationPlan(
           const std::map<std::shared_ptr<ServerlessComputeNode>,
                          std::shared_ptr<ServerlessSchedulingDecisions>>& plans) = 0;

        /**
        * @brief Select the node that has the best layer-load plan
        * @param plans a map of plans
        * @return a compute node
        */
        virtual std::shared_ptr<ServerlessComputeNode> pickBestLayerLoadPlan(
            const std::map<std::shared_ptr<ServerlessComputeNode>,
                           std::shared_ptr<ServerlessSchedulingDecisions>>& plans) = 0;

        /**
        * @brief Select the node that has the best layer-copy plan
        * @param plans a map of plans
        * @return a compute node
        */
        virtual std::shared_ptr<ServerlessComputeNode> pickBestLayerCopyPlan(
            const std::map<std::shared_ptr<ServerlessComputeNode>,
                           std::shared_ptr<ServerlessSchedulingDecisions>>& plans) = 0;

    };
}

#endif //SERVERLESSPLANSELECTIONPOLICY_H
