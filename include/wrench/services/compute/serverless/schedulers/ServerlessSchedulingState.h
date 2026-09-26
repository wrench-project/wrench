#ifndef WRENCH_SERVERLESSSCHEDULINGSTATE_H
#define WRENCH_SERVERLESSSCHEDULINGSTATE_H

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
    class ImageLayer;
    class Invocation;

    /***********************/
    /** \cond INTERNAL    **/
    /***********************/

    /**
     * @brief A class to encode a "scheduling state", that is a clone of the system state that
     *        a scheduler can work with and modify at will for planning purposes
     */
    class ServerlessSchedulingState {

    public:
        explicit ServerlessSchedulingState(const ServerlessStateOfTheSystem* state,
            const std::vector<std::shared_ptr<Invocation>>&schedulable_invocations);

        ~ServerlessSchedulingState() = default;

	    /** @brief The compute nodes */
        std::vector<std::shared_ptr<ServerlessComputeNode>> _compute_nodes;

	    /** @brief Map of core availability */
        std::map<std::shared_ptr<ServerlessComputeNode>, unsigned int> _cores_available;
        /** @brief Map of RAM availability */
        std::map<std::shared_ptr<ServerlessComputeNode>, sg_size_t> _ram_available;
        /** @brief Map of disk availability */
        std::map<std::shared_ptr<ServerlessComputeNode>, sg_size_t> _disk_available;

	    /** @brief Map of idle containers */
        std::map<std::shared_ptr<ServerlessComputeNode>, std::set<std::shared_ptr<Container>>> _idle_containers;

        /** @brief Map of running containers */
        std::map<std::shared_ptr<ServerlessComputeNode>, std::set<std::shared_ptr<Container>>> _busy_containers;

	    /** @brief Map of on-disk image layers */
        std::map<std::shared_ptr<ServerlessComputeNode>, std::set<std::shared_ptr<ImageLayer>>> _image_layers_on_disk;
	    /** @brief Map of soon-to-be-on-disk image layers */
        std::map<std::shared_ptr<ServerlessComputeNode>, std::set<std::shared_ptr<ImageLayer>>> _image_layers_on_their_way_to_disk;

	    /** @brief Map of in-RAM image layers */
        std::map<std::shared_ptr<ServerlessComputeNode>, std::set<std::shared_ptr<ImageLayer>>> _image_layers_in_ram;
	    /** @brief Map of soon-to-be-in-RAM image layers */
        std::map<std::shared_ptr<ServerlessComputeNode>, std::set<std::shared_ptr<ImageLayer>>> _image_layers_on_their_way_to_ram;

        /** @brief Map of layers that should not be evicted, due to previous scheduling decisions */
        std::map<std::shared_ptr<ServerlessComputeNode>, std::set<std::shared_ptr<ImageLayer>>> _protected_layers;

        // HELPER FUNCTIONS
        bool areAllImageLayersInRAM(const std::shared_ptr<Image> &image, const std::shared_ptr<ServerlessComputeNode> &node) const;
        bool areAllImageLayersInRAMOrOnTheirWayToRAM(const std::shared_ptr<Image> &image, const std::shared_ptr<ServerlessComputeNode> &node) const;
        bool areAllImageLayersOnDiskOrOnTheirWayToRAM(const std::shared_ptr<Image> &image, const std::shared_ptr<ServerlessComputeNode> &node) const;
        bool areAllImageLayersOnDisk(const std::shared_ptr<Image> &image, const std::shared_ptr<ServerlessComputeNode> &node) const;
        bool areAllImageLayersOnDiskOrOnTheirWayToDisk(const std::shared_ptr<Image> &image, const std::shared_ptr<ServerlessComputeNode> &node) const;
    };

        /***********************/
        /** \endcond          **/
        /***********************/
}

#endif //WRENCH_SERVERLESSSCHEDULINGSTATE_H
