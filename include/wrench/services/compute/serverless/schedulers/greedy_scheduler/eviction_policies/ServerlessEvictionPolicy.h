#ifndef SERVERLESSEVICTIONPOLICY_H
#define SERVERLESSEVICTIONPOLICY_H

#include <vector>
#include <memory>
#include <set>

#include <simgrid/forward.h>


namespace wrench {
    class Container;
    class ImageLayer;
    class ServerlessSchedulingState;
    class ServerlessComputeNode;
    class LRUServerlessInvocationOrderingPolicy;
    class FewestServerlessInvocationOrderingPolicy;

    /**
     * @brief An abstract class to implement a serverless eviction policy
     */
    class ServerlessEvictionPolicy {
    public:
        ServerlessEvictionPolicy() = default;;
        virtual ~ServerlessEvictionPolicy() = default;

        /**
         * @brief Method to pick idle containers to evict using the "FEWEST" policy
         * @param idle_containers List of (idle) containers to sort
         * @param ram_space_to_free_up Amount of RAM to free up
         * @param disk_space_to_free_up Amount of disk to free up
         * @return A sorted list of (idle) containers
         */
        virtual std::set<std::shared_ptr<Container>> pickIdleContainersForEviction(
            const std::set<std::shared_ptr<Container>>& idle_containers,
            sg_size_t& ram_space_to_free_up,
            sg_size_t& disk_space_to_free_up) = 0;

        /**
         * @brief Method to pick layers to evict from RAM using the "Fewest" policy
         * @param node the compute node where the evictions are to take place
         * @param layers candidate victim layers
         * @param ram_space_to_free_up Amount of RAM to free up
         * @param disk_space_to_free_up Amount of disk to free up
         * @return
         */
        virtual std::set<std::shared_ptr<ImageLayer>> pickImageLayersForRAMEviction(
            const std::shared_ptr<ServerlessComputeNode>& node,
            const std::set<std::shared_ptr<ImageLayer>>& layers,
            sg_size_t& ram_space_to_free_up,
            sg_size_t& disk_space_to_free_up) = 0;

        /**
         * @brief Method to pick layers to evict from disk using the "Fewest" policy
         * @param node the compute node where the evictions are to take place
         * @param layers candidate victim layers
         * @param ram_space_to_free_up Amount of Disk to free up
         * @param disk_space_to_free_up Amount of disk to free up
         * @return
         */
        virtual std::set<std::shared_ptr<ImageLayer>> pickImageLayersForDiskEviction(
            const std::shared_ptr<ServerlessComputeNode>& node,
            const std::set<std::shared_ptr<ImageLayer>>& layers,
            sg_size_t& ram_space_to_free_up,
            sg_size_t& disk_space_to_free_up) = 0;
    };
}

#endif //SERVERLESSEVICTIONPOLICY_H
