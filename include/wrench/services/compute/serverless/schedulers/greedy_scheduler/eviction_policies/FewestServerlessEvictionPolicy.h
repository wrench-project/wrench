#ifndef FEWESTSERVERLESSEVICTIONPOLICY_H
#define FEWESTSERVERLESSEVICTIONPOLICY_H

#include <vector>
#include <memory>
#include <set>

#include <simgrid/forward.h>

#include <wrench/services/compute/serverless/schedulers/greedy_scheduler/eviction_policies/ServerlessEvictionPolicy.h>


namespace wrench {

    class Container;
    class ImageLayer;
    class ServerlessSchedulingState;
    class ServerlessComputeNode;

    /**
     * @brief An eviction policy that uses a heuristic to evict the smallest number of  items
     *        (idle containers, in-RAM layers, on-disk layers)
     */
    class FewestServerlessEvictionPolicy : public ServerlessEvictionPolicy {
    public:

        FewestServerlessEvictionPolicy() = default;
        ~FewestServerlessEvictionPolicy() override = default;

        std::set<std::shared_ptr<Container>> pickIdleContainersForEviction(
            const std::set<std::shared_ptr<Container>>& idle_containers,
            sg_size_t& ram_space_to_free_up,
            sg_size_t& disk_space_to_free_up) override;

        std::set<std::shared_ptr<ImageLayer>> pickImageLayersForRAMEviction(
            const std::shared_ptr<ServerlessComputeNode>& node,
            const std::set<std::shared_ptr<ImageLayer>>& layers,
            sg_size_t& ram_space_to_free_up,
            sg_size_t& disk_space_to_free_up) override;

        std::set<std::shared_ptr<ImageLayer>> pickImageLayersForDiskEviction(
            const std::shared_ptr<ServerlessComputeNode>& node,
            const std::set<std::shared_ptr<ImageLayer>>& layers,
            sg_size_t& ram_space_to_free_up,
            sg_size_t& disk_space_to_free_up) override;
    };
}

#endif //FEWESTSERVERLESSEVICTIONPOLICY_H
