#ifndef WRENCH_GREEDYSCHEDULINGSTATE_H
#define WRENCH_GREEDYSCHEDULINGSTATE_H

#include <map>
#include <unordered_set>
#include <simgrid/forward.h>

namespace wrench {
    class ServerlessStateOfTheSystem;
    class ServerlessComputeNode;
    class Container;
    class Image;
    class Invocation;

    class GreedySchedulingState {
        /***********************/
        /** \cond INTERNAL    **/
        /***********************/


    public:
        explicit GreedySchedulingState(const ServerlessStateOfTheSystem* state,
            const std::vector<std::shared_ptr<Invocation>>&schedulable_invocations);

        ~GreedySchedulingState() = default;

        std::vector<std::shared_ptr<ServerlessComputeNode>> compute_nodes;

        std::map<std::shared_ptr<ServerlessComputeNode>, unsigned int> cores_available;
        // std::map<std::shared_ptr<ServerlessComputeNode>, sg_size_t> ram_available;
        // std::map<std::shared_ptr<ServerlessComputeNode>, sg_size_t> disk_available;

        std::map<std::shared_ptr<ServerlessComputeNode>, std::set<std::shared_ptr<Container>>> idle_containers;

        std::map<std::shared_ptr<ServerlessComputeNode>, std::unordered_set<std::shared_ptr<Image>>> images_on_disk;
        std::map<std::shared_ptr<ServerlessComputeNode>, std::unordered_set<std::shared_ptr<Image>>> images_on_their_way_to_disk;

        std::map<std::shared_ptr<ServerlessComputeNode>, std::unordered_set<std::shared_ptr<Image>>> images_in_ram;
        std::map<std::shared_ptr<ServerlessComputeNode>, std::unordered_set<std::shared_ptr<Image>>> images_on_their_way_to_ram;

        /***********************/
        /** \endcond          **/
        /***********************/
    };
}

#endif //WRENCH_GREEDYSCHEDULINGSTATE_H
