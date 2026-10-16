#ifndef WRENCH_SERVERLESSSCHEDULER_H
#define WRENCH_SERVERLESSSCHEDULER_H

#include <wrench/function/Invocation.h>
#include <wrench/function/Image.h>
#include <wrench/function/ImageLayer.h>
#include <wrench/services/compute/serverless/ServerlessStateOfTheSystem.h>
#include <wrench/services/compute/serverless/Container.h>
#include <vector>
#include <string>

namespace wrench {
    /***********************/
    /** \cond INTERNAL    **/
    /***********************/

    /**
     * @brief A structure to encode a compute node activations decision
     */
    struct ActivateComputeNodes {
	/**
	 * @brief The number of compute nodes to activate
	 */
        unsigned long num_compute_nodes;
    };

    /**
    * @brief A structure to encode an invocation fail decision
    */
    struct FailInvocation {
        /**
         * @brief The invocation to fail
         */
        std::shared_ptr<Invocation> invocation;
    };

    /**
     * @brief A structure to encode an idle container termination scheduling decision
     */
    struct TerminateIdleContainer {
        /**
         * @brief The idle container to terminate
         */
        std::shared_ptr<Container> container;
    };

    /**
     * @brief A structure to encode a container prewarm scheduling decision
     */
    struct PrewarmContainer {
        /**
         * @brief The function for which a container should be pre-warmed
         */
        std::shared_ptr<Function> function;
        /**
         * @brief The compute node where to start the container
         */
        std::shared_ptr<ServerlessComputeNode> compute_node;
    };

    /**
     * @brief A structure to encode an image-layer-copy scheduling decision
     */
    struct CopyImageLayer {
        /**
         * @brief The image layer to copy
         */
        std::shared_ptr<ImageLayer> layer;
        /**
         * @brief The compute node to copy the image layer to
         */
        std::shared_ptr<ServerlessComputeNode> compute_node;
    };

    /**
     * @brief A structure to encode an image-layer-load scheduling decision
     */
    struct LoadImageLayer {
        /**
         * @brief The image layer to load
         */
        std::shared_ptr<ImageLayer> layer;
        /**
         * @brief The compute node to load the image at
         */
        std::shared_ptr<ServerlessComputeNode> compute_node;
    };

    /**
     * @brief A structure to encode an image-dispatch scheduling decision
     */
    struct DispatchInvocation {
        /**
         * @brief The invocation to dispatch
         */
        std::shared_ptr<Invocation> invocation;
        /**
         * @brief The compute node where to dispatch
         */
        std::shared_ptr<ServerlessComputeNode> compute_node;
        /**
         * @brief The container to use (nullptr if new container is to be started)
         */
        std::shared_ptr<Container> container;
    };

    /**
     * @brief A structure to encode a layer eviction from RAM scheduling decision
     */
    struct EvictLayerFromRAM {
        /**
         * @brief The layer to evict
         */
        std::shared_ptr<ImageLayer> layer;
        /**
         * @brief The compute node where to evict
         */
        std::shared_ptr<ServerlessComputeNode> compute_node;
    };

    /**
     * @brief A structure to encode a layer eviction from disk scheduling decision
     */
    struct EvictLayerFromDisk {
        /**
         * @brief The layer to evict
         */
        std::shared_ptr<ImageLayer> layer;
        /**
         * @brief The compute node where to evict
         */
        std::shared_ptr<ServerlessComputeNode> compute_node;
    };


    /**
     * @brief A data structure that stores all scheduling decisions made by a serverless scheduler:
     *        - How many compute nodes to activate
     *        - Which invocations to fail
     *        - Which idle container to terminate
     *        - Which layer to evict from RAM
     *        - Which layer to evict from disk
     *        - Which images should be copied from the head node to compute nodes' disks, initiated right now
     *        - Which images should be loaded into compute node's RAMs, initiated right now
     *        - Which invocations should be dispatched right now
     */
    class ServerlessSchedulingDecisions {
    public:
        /** @brief The list of compute node activations  */
        std::vector<ActivateComputeNodes> compute_node_activations;
        /** @brief The list of invocations to fail  */
        std::vector<FailInvocation> invocation_fails;
        /** @brief The list of idle container terminations  */
        std::vector<TerminateIdleContainer> idle_container_terminations;
        /** @brief The list of layer from RAM evictions */
        std::vector<EvictLayerFromRAM> layer_evictions_from_ram;
        /** @brief The list of layer from disk evictions */
        std::vector<EvictLayerFromDisk> layer_evictions_from_disk;
        /** @brief The list of image layer copies to storage at compute nodes */
        std::vector<CopyImageLayer> image_layer_copies_to_disk;
        /** @brief The list of image layer loads in RAM at compute nodes */
        std::vector<LoadImageLayer> image_layer_loads_to_RAM;
        /** @brief The list of function invocations at compute nodes */
        std::vector<DispatchInvocation> invocation_dispatches;
        /** @brief The list of container prewarms at compute nodes */
        std::vector<PrewarmContainer> container_prewarms;

        /** @brief The (optional) next scheduler wake-up time */
        std::optional<double> scheduler_wakeup_date;

        /**
         * @brief Method to print scheduling decisions
         */
        void print() {
            if (compute_node_activations.empty() and
                invocation_fails.empty() and
                idle_container_terminations.empty() and
                layer_evictions_from_ram.empty() and
                layer_evictions_from_disk.empty() and
                image_layer_copies_to_disk.empty() and
                image_layer_loads_to_RAM.empty() and
                invocation_dispatches.empty() and
                container_prewarms.empty()) {
                std::cerr << "** NO SCHEDULING DECISIONS **" << std::endl;
                return;
            }

            std::cerr << "** SCHEDULING DECISIONS **" << std::endl;
            for (const auto& [num_compute_nodes] : compute_node_activations) {
                std::cerr << "  Compute node activations: " << num_compute_nodes <<  " compute nodes activated" << std::endl;
            }
            for (const auto& [invocation] : invocation_fails) {
                std::cerr << "  Forceful invocation fail: for function " << invocation->getFunction()->getName() << std::endl;
            }
            for (const auto& [container] : idle_container_terminations) {
                std::cerr << "  Idle container termination: for function " << container->getFunction()->getName() <<
                    " at " << container->getComputeNode()->hostname << std::endl;
            }
            for (const auto& [layer, node] : layer_evictions_from_ram) {
                std::cerr << "  Layer eviction from RAM: for layer " << layer->getName() << " at " << node->hostname <<
                    std::endl;
            }
            for (const auto& [layer, node] : layer_evictions_from_disk) {
                std::cerr << "  Layer eviction from disk: for layer " << layer->getName() << " at " << node->hostname <<
                    std::endl;
            }
            for (const auto& [layer, node] : image_layer_copies_to_disk) {
                std::cerr << "  Image layer copy: " << layer->getName() << " at " << node->hostname << std::endl;
            }
            for (const auto& [layer, node] : image_layer_loads_to_RAM) {
                std::cerr << "  Image layer load: " << layer->getName() << " at " << node->hostname << std::endl;
            }
            for (const auto& [invocation, node, container] : invocation_dispatches) {
                std::cerr << "  Invocation dispatch: for " << invocation->getFunction()->getName() <<
                    " at " << node->hostname << " (" << (container ? "on an idle container" : "on a new container")
                    << ")" << std::endl;
            }
            for (const auto& [function, node] : container_prewarms) {
                std::cerr << "  Container prewarm: for " << function->getName() <<
                    " at " << node->hostname << std::endl;
            }
        }
    };

    /**
     * @brief Abstract base class for scheduling in a serverless compute service.
     */
    class ServerlessScheduler {
    public:

        ServerlessScheduler() = default;

        virtual ~ServerlessScheduler() = default;

        /**
         * @brief Given the list of schedulable invocations and the current system state, decide:
         *   - how many compute nodes to activate
         *   - which invocations to forcefully fail
         *   - which idle containers to terminate
         *   - which layers to evict from RAM
         *   - which layers to evict from disk
         *   - which layers to copy to compute nodes
         *   - which layers to load into memory at compute nodes
         *   - which invocations to dispatch at compute nodes
         *   - which containers to prewarm at compute nodes
         *
         *   A scheduling decision is an intent, not a reservation. The serverless compute service checks
         *   whether the decision can actually be executed when it processes it.
         *
         * @param schedulable_invocations A list of invocations for which all necessary layers reside on the head node
         * @param state The current system state
         * @return A SchedulingDecisions object
         */
        virtual std::shared_ptr<ServerlessSchedulingDecisions> schedule(
            const std::vector<std::shared_ptr<Invocation>> schedulable_invocations,
            const ServerlessStateOfTheSystem* state
        ) = 0;

    };

    /***********************/
    /** \endcond          **/
    /***********************/
} // namespace wrench

#endif // WRENCH_SERVERLESSSCHEDULER_H
