#ifndef WRENCH_GREEDYSCHEDULINGSTATE_H
#define WRENCH_GREEDYSCHEDULINGSTATE_H

#include <map>
#include <set>
#include <memory>
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

	/** @brief The compute nodes */
        std::vector<std::shared_ptr<ServerlessComputeNode>> compute_nodes;

	/** @brief Map of core availability */
        std::map<std::shared_ptr<ServerlessComputeNode>, unsigned int> cores_available;

	/** @brief Map of idle containers */
        std::map<std::shared_ptr<ServerlessComputeNode>, std::set<std::shared_ptr<Container>>> idle_containers;

	/** @brief Map of on-disk images */
        std::map<std::shared_ptr<ServerlessComputeNode>, std::unordered_set<std::shared_ptr<Image>>> images_on_disk;
	/** @brief Map of soon-to-be-on-disk images */
        std::map<std::shared_ptr<ServerlessComputeNode>, std::unordered_set<std::shared_ptr<Image>>> images_on_their_way_to_disk;

	/** @brief Map of in-RAM images */
        std::map<std::shared_ptr<ServerlessComputeNode>, std::unordered_set<std::shared_ptr<Image>>> images_in_ram;
	/** @brief Map of soon-to-be-in-RAM images */
        std::map<std::shared_ptr<ServerlessComputeNode>, std::unordered_set<std::shared_ptr<Image>>> images_on_their_way_to_ram;

        /***********************/
        /** \endcond          **/
        /***********************/
    };
}

#endif //WRENCH_GREEDYSCHEDULINGSTATE_H
