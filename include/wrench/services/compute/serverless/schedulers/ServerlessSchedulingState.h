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
    class Function;
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

    	/** @brief The schedulable invocations (a copy, can be modified at will) */
    	std::vector<std::shared_ptr<Invocation>> _schedulable_invocations;

    	/** @brief Per-function counts of outstanding invocations */
    	std::map<std::shared_ptr<Function>, unsigned long> _outstanding_invocation_counts;

	    /** @brief The activated compute nodes */
        std::vector<std::shared_ptr<ServerlessComputeNode>> _activated_compute_nodes;

    	/** @brief The number of deactivated compute nodes */
    	unsigned long _num_deactivated_compute_nodes;

    	/** @brief The number of compute nodes in the process of being activated */
    	unsigned long _num_being_activated_compute_nodes;

    	/** @brief Number of possible new invocations that could be started on compute node */
    	std::map<std::shared_ptr<ServerlessComputeNode>, unsigned long> _num_possible_new_invocations_on_compute_node;

    	/** @brief Number of possible new containers that could be started on compute node */
    	std::map<std::shared_ptr<ServerlessComputeNode>, unsigned long> _num_possible_new_containers_on_compute_node;

    	/** @brief Number of possible new invocations that could be started on a current container */
    	std::map<std::shared_ptr<Container>, unsigned long> _num_possible_new_invocations_on_container;

    	/**
		 * @brief Planned in-flight invocation counts for existing initialized
		 *        containers. Includes observed invocations and dispatches selected
		 *        earlier in this scheduling round.
		 *
		 * Does not contain initializing containers or containers whose creation
		 * has only been proposed in this round.
		 */
    	std::map<std::shared_ptr<Container>, unsigned long> _num_in_flight_invocations_on_container;

        /** @brief Map of RAM availability */
        std::map<std::shared_ptr<ServerlessComputeNode>, sg_size_t> _ram_available;

        /** @brief Map of disk availability */
        std::map<std::shared_ptr<ServerlessComputeNode>, sg_size_t> _disk_available;

	    /** @brief Map of initializing containers */
        std::map<std::shared_ptr<ServerlessComputeNode>, std::set<std::shared_ptr<Container>>> _initializing_containers;

        /** @brief Map of initialized containers */
        std::map<std::shared_ptr<ServerlessComputeNode>, std::set<std::shared_ptr<Container>>> _initialized_containers;

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
        [[nodiscard]] bool areAllImageLayersInRAM(const std::shared_ptr<Image> &image, const std::shared_ptr<ServerlessComputeNode> &node) const;
        [[nodiscard]] bool areAllImageLayersInRAMOrOnTheirWayToRAM(const std::shared_ptr<Image> &image, const std::shared_ptr<ServerlessComputeNode> &node) const;
        [[nodiscard]] bool areAllImageLayersOnDiskOrOnTheirWayToRAM(const std::shared_ptr<Image> &image, const std::shared_ptr<ServerlessComputeNode> &node) const;
        [[nodiscard]] bool areAllImageLayersOnDisk(const std::shared_ptr<Image> &image, const std::shared_ptr<ServerlessComputeNode> &node) const;
        [[nodiscard]] bool areAllImageLayersOnDiskOrOnTheirWayToDisk(const std::shared_ptr<Image> &image, const std::shared_ptr<ServerlessComputeNode> &node) const;

    	[[nodiscard]] std::set<std::shared_ptr<Container>> getIdleContainersAtComputeNode(const std::shared_ptr<ServerlessComputeNode> &node) const;
    };

        /***********************/
        /** \endcond          **/
        /***********************/
}

#endif //WRENCH_SERVERLESSSCHEDULINGSTATE_H
