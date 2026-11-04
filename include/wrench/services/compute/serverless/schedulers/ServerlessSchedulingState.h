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
        explicit ServerlessSchedulingState(const ServerlessStateOfTheSystem* state);

        ~ServerlessSchedulingState() = default;

    	/**
		* Accessor Functions
		*/
    	std::vector<std::shared_ptr<Invocation>>& getSchedulableInvocations();
    	const std::map<std::shared_ptr<Function>, unsigned long>& getPendingInvocationCounts() const;
    	unsigned long getNumInFlightInvocationsOnContainer(const std::shared_ptr<Container>& container) const;
    	[[nodiscard]] const std::vector<std::shared_ptr<ServerlessComputeNode>>& getActivatedComputeNodes();
    	[[nodiscard]] unsigned long getNumBeingActivatedComputeNodes() const;
    	[[nodiscard]] unsigned long getNumDeactivatedComputeNodes() const;
    	const std::set<std::shared_ptr<Container>>& getInitializedContainers(const std::shared_ptr<ServerlessComputeNode>& node);
    	const std::set<std::shared_ptr<Container>>& getInitializingContainers(const std::shared_ptr<ServerlessComputeNode>& node);
    	unsigned long getNumPossibleNewInvocationsAtComputeNode(const std::shared_ptr<ServerlessComputeNode>& node) const;
    	unsigned long getNumPossibleNewInvocationsOnContainer(const std::shared_ptr<Container>& container) const;
    	unsigned long getNumPossibleNewContainersAtComputeNode(const std::shared_ptr<ServerlessComputeNode>& node) const;
    	const std::set<std::shared_ptr<ImageLayer>>& getProtectedImageLayers(const std::shared_ptr<ServerlessComputeNode>& node);
    	const std::set<std::shared_ptr<ImageLayer>>& getImageLayersOnDisk(const std::shared_ptr<ServerlessComputeNode>& node);
    	const std::set<std::shared_ptr<ImageLayer>>& getImageLayersOnTheirWayToDisk(const std::shared_ptr<ServerlessComputeNode>& node);
    	const std::set<std::shared_ptr<ImageLayer>>& getImageLayersInRAM(const std::shared_ptr<ServerlessComputeNode>& node);
    	const std::set<std::shared_ptr<ImageLayer>>& getImageLayersOnTheirWayToRAM(const std::shared_ptr<ServerlessComputeNode>& node) const;
    	[[nodiscard]] sg_size_t getRAMSpaceAvailable(const std::shared_ptr<ServerlessComputeNode>& node) const;
    	[[nodiscard]] sg_size_t getDiskSpaceAvailable(const std::shared_ptr<ServerlessComputeNode>& node) const;

    	/**
		 * Update this planning snapshot to account for already-selected operations.
		 *
		 * These methods do not select operations, append scheduling decisions,
		 * execute simulated work, or modify runtime objects. The caller remains
		 * responsible for adding the corresponding decisions to the returned batch.
		 */
    	void excludeNodeFromPlanning(const std::shared_ptr<ServerlessComputeNode>& node);
    	void planInvocationFailures(const std::set<std::shared_ptr<Invocation>>& invocations);
    	void protectImageLayers(const std::shared_ptr<ServerlessComputeNode>& node, const std::shared_ptr<Image>& image);
    	void planIdleContainerTermination(const std::shared_ptr<ServerlessComputeNode>& node, const std::shared_ptr<Container>& container);
    	void planLayerEvictionFromRAM(const std::shared_ptr<ServerlessComputeNode>& node, const std::shared_ptr<ImageLayer>& layer);
    	void planLayerEvictionFromDisk(const std::shared_ptr<ServerlessComputeNode>& node, const std::shared_ptr<ImageLayer>& layer);
    	void planInvocationDispatch(const std::shared_ptr<Invocation>& inv, const std::shared_ptr<ServerlessComputeNode>& node, const std::shared_ptr<Container>& container);
    	void planLayerLoadToRAM(const std::shared_ptr<ServerlessComputeNode>& node, const std::shared_ptr<ImageLayer>& layer);
    	void planLayerCopyToDisk(const std::shared_ptr<ServerlessComputeNode>& node, const std::shared_ptr<ImageLayer>& layer);
    	void planContainerPrewarm(const std::shared_ptr<ServerlessComputeNode>& node, const std::shared_ptr<Function>& function);

        /**
         * Helper Functions
         */
        [[nodiscard]] bool areAllImageLayersInRAM(const std::shared_ptr<Image> &image, const std::shared_ptr<ServerlessComputeNode> &node) const;
    	[[nodiscard]] bool areAllImageLayersInRAMOrOnTheirWayToRAM(const std::shared_ptr<Image> &image, const std::shared_ptr<ServerlessComputeNode> &node) const;
    	[[nodiscard]] bool areAllImageLayersOnDiskOrOnTheirWayToRAM(const std::shared_ptr<Image> &image, const std::shared_ptr<ServerlessComputeNode> &node) const;
    	[[nodiscard]] bool areAllImageLayersOnDisk(const std::shared_ptr<Image> &image, const std::shared_ptr<ServerlessComputeNode> &node) const;
    	[[nodiscard]] bool areAllImageLayersOnDiskOrOnTheirWayToDisk(const std::shared_ptr<Image> &image, const std::shared_ptr<ServerlessComputeNode> &node) const;
    	[[nodiscard]] std::set<std::shared_ptr<Container>> getIdleContainersAtComputeNode(const std::shared_ptr<ServerlessComputeNode> &node) const;

    private:
    	/** @brief The admitted invocations, i.e., whose image layers have not all yet been downloaded */
    	std::map<std::shared_ptr<Image>, std::vector<std::shared_ptr<Invocation>>> _admitted_invocations;

    	/** @brief The schedulable invocations */
    	std::vector<std::shared_ptr<Invocation>> _schedulable_invocations;

    	/** @brief The per-function pending invocation counts (outstanding minus assigned to containers) */
    	std::map<std::shared_ptr<Function>, unsigned long> _pending_invocation_counts;

	    /** @brief The activated compute nodes */
        std::vector<std::shared_ptr<ServerlessComputeNode>> _activated_compute_nodes;

    	/** @brief The number of deactivated compute nodes */
    	unsigned long _num_deactivated_compute_nodes;

    	/** @brief The number of compute nodes in the process of being activated */
    	unsigned long _num_being_activated_compute_nodes;


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


    	struct NodePlanningState {
    		unsigned long _num_possible_new_invocations;
    		unsigned long _num_possible_new_containers;
    		sg_size_t _ram_available;
    		sg_size_t _disk_available;
    		std::set<std::shared_ptr<Container>> _initializing_containers;
    		std::set<std::shared_ptr<Container>> _initialized_containers;
    		std::set<std::shared_ptr<ImageLayer>> _image_layers_on_disk;
			std::set<std::shared_ptr<ImageLayer>> _image_layers_on_their_way_to_disk;
			std::set<std::shared_ptr<ImageLayer>> _image_layers_in_ram;
			std::set<std::shared_ptr<ImageLayer>> _image_layers_on_their_way_to_ram;
			std::set<std::shared_ptr<ImageLayer>> _protected_layers;
    	};

    	std::map<std::shared_ptr<ServerlessComputeNode>, NodePlanningState> _node_states;

    	// /** @brief Number of possible new invocations that could be started on compute node */
    	// std::map<std::shared_ptr<ServerlessComputeNode>, unsigned long> _num_possible_new_invocations_on_compute_node;
     //
    	// /** @brief Number of possible new containers that could be started on compute node */
    	// std::map<std::shared_ptr<ServerlessComputeNode>, unsigned long> _num_possible_new_containers_on_compute_node;
     //
    	// /** @brief Map of RAM availability */
     //    std::map<std::shared_ptr<ServerlessComputeNode>, sg_size_t> _ram_available;
     //
     //    /** @brief Map of disk availability */
     //    std::map<std::shared_ptr<ServerlessComputeNode>, sg_size_t> _disk_available;
     //
	    // /** @brief Map of initializing containers */
     //    std::map<std::shared_ptr<ServerlessComputeNode>, std::set<std::shared_ptr<Container>>> _initializing_containers;
     //
     //    /** @brief Map of initialized containers */
     //    std::map<std::shared_ptr<ServerlessComputeNode>, std::set<std::shared_ptr<Container>>> _initialized_containers;
     //
	    // /** @brief Map of on-disk image layers */
     //    std::map<std::shared_ptr<ServerlessComputeNode>, std::set<std::shared_ptr<ImageLayer>>> _image_layers_on_disk;
     //
	    // /** @brief Map of soon-to-be-on-disk image layers */
     //    std::map<std::shared_ptr<ServerlessComputeNode>, std::set<std::shared_ptr<ImageLayer>>> _image_layers_on_their_way_to_disk;
     //
	    // /** @brief Map of in-RAM image layers */
     //    std::map<std::shared_ptr<ServerlessComputeNode>, std::set<std::shared_ptr<ImageLayer>>> _image_layers_in_ram;
     //
	    // /** @brief Map of soon-to-be-in-RAM image layers */
     //    std::map<std::shared_ptr<ServerlessComputeNode>, std::set<std::shared_ptr<ImageLayer>>> _image_layers_on_their_way_to_ram;
     //
     //    /** @brief Map of layers that should not be evicted, due to previous scheduling decisions */
     //    std::map<std::shared_ptr<ServerlessComputeNode>, std::set<std::shared_ptr<ImageLayer>>> _protected_layers;


    };

        /***********************/
        /** \endcond          **/
        /***********************/
}

#endif //WRENCH_SERVERLESSSCHEDULINGSTATE_H
