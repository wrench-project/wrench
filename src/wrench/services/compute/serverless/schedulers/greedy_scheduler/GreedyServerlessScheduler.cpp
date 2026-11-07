#include "wrench.h"


#include <utility>
#include <algorithm>
#include <memory>
#include <set>
#include <vector>

#include "wrench/services/compute/serverless/schedulers/greedy_scheduler/GreedyServerlessScheduler.h"
#include "wrench/services/compute/serverless/schedulers/ServerlessSchedulingState.h"
#include "wrench/logging/TerminalOutput.h"


WRENCH_LOG_CATEGORY(wrench_greedy_serverless_scheduler, "Log category for GreedyServerlessScheduler");

namespace wrench {

    /**
     * @brief Constructor. Creates a greedy scheduling with the following default policies (use setPolicy() to change them):
     *   - Invocation ordering policy: FCFSServerlessInvocationOrderingPolicy
     *   - Node selection policy: SelectAllPossibleServerlessNodeSelectionPolicy
     *   - Eviction policy: FewestServerlessEvictionPolicy
     *   - Plan selection policy: EvictionAverseServerlessPlanSelectionPolicy
     *   - Node scaling policy: ActivateOneWhenNoFreeSlotServerlessNodeActivationPolicy
     *   - Container prewarm policy: NeverPrewarmServerlessContainerPrewarmPolicy
     *   - Invocation failing policy: NeverFailServerlessInvocationFailingPolicy
     */
    GreedyServerlessScheduler::GreedyServerlessScheduler() {
        _invocation_ordering_policy = std::make_shared<FCFSInvocationOrderingPolicy>();
        _node_selection_policy = std::make_shared<AllPossibleNodeSelectionPolicy>();
        _eviction_policy = std::make_shared<FewestServerlessEvictionPolicy>();
        _plan_selection_policy = std::make_shared<EvictionAversePlanSelectionPolicy>();
        _node_scaling_policy = std::make_shared<ReactiveScaleUpNodeActivationPolicy>();
        _container_prewarm_policy = std::make_shared<NeverPrewarmContainerPolicy>();
        _invocation_failing_policy = std::make_shared<NeverFailInvocationsPolicy>();
        _reusable_container_selection_policy = std::make_shared<
            ExpiringLatestReusableContainerSelectionPolicy>();
    }

    /**
     * @brief Get the scheduler's node activation policy
     * @return the policy
     */
    ServerlessNodeScalingPolicy* GreedyServerlessScheduler::getNodeScalingPolicy() const {
        return _node_scaling_policy.get();
    }

    /**
     * @brief Get the scheduler's invocation ordering policy
     * @return the policy
     */
    ServerlessInvocationOrderingPolicy* GreedyServerlessScheduler::getInvocationOrderingPolicy() const {
        return _invocation_ordering_policy.get();
    }

    /**
     * @brief Get the schedule's node selection policy
     * @return the policy
     */
    ServerlessNodeSelectionPolicy* GreedyServerlessScheduler::getNodeSelectionPolicy() const {
        return _node_selection_policy.get();
    }

    /**
     * @brief Get the scheduler's eviction policy
     * @return the policy
     */
    ServerlessEvictionPolicy* GreedyServerlessScheduler::getEvictionPolicy() const {
        return _eviction_policy.get();
    }

    /**
     * @brief Get the scheduler's plan selection policy
     * @return the policy
     */
    ServerlessPlanSelectionPolicy* GreedyServerlessScheduler::getPlanSelectionPolicy() const {
        return _plan_selection_policy.get();
    }

    /**
     * @brief Get the scheduler's container prewarm policy
     * @return the policy
     */
    ServerlessContainerPrewarmPolicy* GreedyServerlessScheduler::getContainerPrewarmPolicy() const {
        return _container_prewarm_policy.get();
    }

    /**
     * @brief Get the scheduler's invocation failing policy
     * @return the policy
     */
    ServerlessInvocationFailingPolicy* GreedyServerlessScheduler::getInvocationFailingPolicy() const {
        return _invocation_failing_policy.get();
    }

    /**
     * @brief Get the scheduler's idle container selection policy
     * @return the policy
     */
    ServerlessReusableContainerSelectionPolicy* GreedyServerlessScheduler::getReusableContainerSelectionPolicy() const {
        return _reusable_container_selection_policy.get();
    }

    /**
     * @brief Policy setter
     * @param policy A node activation policy (at your own risk once the simulation has started)
     */
    void GreedyServerlessScheduler::setPolicy(std::shared_ptr<ServerlessNodeScalingPolicy> policy) {
        _node_scaling_policy = std::move(policy);
    }

    /**
     * @brief Policy setter
     * @param policy An invocation ordering policy (at your own risk once the simulation has started)
     */
    void GreedyServerlessScheduler::setPolicy(std::shared_ptr<ServerlessInvocationOrderingPolicy> policy) {
        _invocation_ordering_policy = std::move(policy);
    }

    /**
     * @brief Policy setter
     * @param policy A node selection policy (at your own risk once the simulation has started)
     */
    void GreedyServerlessScheduler::setPolicy(std::shared_ptr<ServerlessNodeSelectionPolicy> policy) {
        _node_selection_policy = std::move(policy);
    }

    /**
     * @brief Policy setter
     * @param policy A serverless eviction policy (at your own risk once the simulation has started)
     */
    void GreedyServerlessScheduler::setPolicy(std::shared_ptr<ServerlessEvictionPolicy> policy) {
        _eviction_policy = std::move(policy);
    }

    /**
     * @brief Policy setter
     * @param policy A plan selection policy (at your own risk once the simulation has started)
     */
    void GreedyServerlessScheduler::setPolicy(std::shared_ptr<ServerlessPlanSelectionPolicy> policy) {
        _plan_selection_policy = std::move(policy);
    }

    /**
     * @brief Policy setter
     * @param policy A container prewarm policy (at your own risk once the simulation has started)
     */
    void GreedyServerlessScheduler::setPolicy(std::shared_ptr<ServerlessContainerPrewarmPolicy> policy) {
        _container_prewarm_policy = std::move(policy);
    }

    /**
     * @brief Policy setter
     * @param policy An invocation failing policy (at your own risk once the simulation has started)
     */
    void GreedyServerlessScheduler::setPolicy(std::shared_ptr<ServerlessInvocationFailingPolicy> policy) {
        _invocation_failing_policy = std::move(policy);
    }

    /**
     * @brief Policy setter
     * @param policy A reusable container selection policy (at your own risk once the simulation has started)
     */
    void GreedyServerlessScheduler::setPolicy(std::shared_ptr<ServerlessReusableContainerSelectionPolicy> policy) {
        _reusable_container_selection_policy = std::move(policy);
    }

    /**
     * @brief Build one batch of scheduling decisions using a mutable planning
     *        snapshot without modifying the supplied invocation vector.
     *
     * Records selected invocation failures, filters and orders the local
     * worklist, requests worker activation, and greedily considers the remaining
     * requests. For each selected dispatch or preparation plan, updates the
     * working resource counts, residency information, and protected layers.
     * Finally evaluates prewarming against that working state and combines
     * the installed policies' requested wakeup dates.
     *
     * A pending invocation need not receive an operation in this round. In
     * particular, choosing to wait for an existing transfer protects its layers
     * but does not reserve invocation capacity or bind it to that node. Staging
     * and load operations do not become completed residency changes during planning.
     *
     * The returned operations are intentions for the service to apply, not
     * executed actions or an atomic reservation. See the class documentation
     * for phase precedence, reusable-container capacity, and planning semantics.
     *
     * @param state Current system state from which the planning snapshot is built.
     * @return A new ServerlessSchedulingDecisions object. Its operation lists may
     *         all be empty, independently of whether a future wakeup is requested.
     */
    std::shared_ptr<ServerlessSchedulingDecisions> GreedyServerlessScheduler::schedule(
        const ServerlessStateOfTheSystem* state) {
        auto decisions = std::make_shared<ServerlessSchedulingDecisions>();

        /** Create a scheduling state **/
        auto scheduling_state = std::make_shared<ServerlessSchedulingState>(state);

        /** Invoke the invocation failing policy **/
        auto to_fail = _invocation_failing_policy->failSchedulableInvocations(scheduling_state);
        for (auto const& inv : to_fail) {
            decisions->invocation_fails.push_back({inv});
        }
        scheduling_state->planInvocationFailures(to_fail);

        /** Invoke the node activation policy **/
        auto [num_nodes_to_activate, nodes_to_deactivate] =
            _node_scaling_policy->makeNodeScalingPlan(scheduling_state);
        // Implement node activations
        if (num_nodes_to_activate) {
            decisions->compute_node_activations.push_back({num_nodes_to_activate});
        }
        // Implement node de-activations and update the bookkeeping
        for (auto const& node : nodes_to_deactivate) {
            decisions->compute_node_deactivations.push_back({node});
            scheduling_state->excludeNodeFromPlanning(node);
        }

        /** Sort the schedulable invocations in place **/
        _invocation_ordering_policy->sortSchedulableInvocations(scheduling_state);

        // Go through the invocations and pick target compute node
        for (const auto& inv : scheduling_state->getSchedulableInvocations()) {
            // Pick a target compute node
            auto [target_node, node_decisions] = this->pickComputeNode(scheduling_state, inv);

            // If no node was found, keep going
            if (!target_node) {
                continue;
            }

            // Make the layers of this invocation protected to avoid eviction by subsequence scheduling decisions
            scheduling_state->protectImageLayers(target_node, inv->getFunction()->getImage());

            // If no node-level decisions, move on (it was just a reservation)
            if (!node_decisions) {
                continue;
            }

            // At this point we have a target node and decisions. We simply go through the decisions and:
            // 1) Update the scheduling state;
            // 2) add them to our global set of decisions

            // Idle container terminations
            for (const auto& [container] : node_decisions->idle_container_terminations) {
                scheduling_state->planIdleContainerTermination(target_node, container);
                decisions->idle_container_terminations.push_back({container});
            }

            // Layer evictions from RAM
            for (const auto& [layer, _] : node_decisions->layer_evictions_from_ram) {
                scheduling_state->planLayerEvictionFromRAM(target_node, layer);
                decisions->layer_evictions_from_ram.push_back({layer, target_node});
            }

            // Layer evictions from disk
            for (const auto& [layer, _] : node_decisions->layer_evictions_from_disk) {
                scheduling_state->planLayerEvictionFromDisk(target_node, layer);
                decisions->layer_evictions_from_disk.push_back({layer, target_node});
            }

            // Invocations
            for (const auto& [_ignore1, _ignore2, container] : node_decisions->invocation_dispatches) {
                decisions->invocation_dispatches.push_back({inv, target_node, container});
                scheduling_state->planInvocationDispatch(inv, target_node, container);
            }

            // Image layer loads
            for (const auto& [layer,_] : node_decisions->image_layer_loads_to_RAM) {
                decisions->image_layer_loads_to_RAM.push_back({layer, target_node});
                scheduling_state->planLayerLoadToRAM(target_node, layer);
            }

            // Image layer copies
            for (const auto& [layer,_] : node_decisions->image_layer_stagings_to_disk) {
                decisions->image_layer_stagings_to_disk.push_back({layer, target_node});
                scheduling_state->planLayerStagingToDisk(target_node, layer);

            }
        }

        // Prewarm containers
        auto to_prewarm = _container_prewarm_policy->prewarmContainers(scheduling_state);
        for (auto const& [function, node] : to_prewarm) {
            decisions->container_prewarms.push_back({function, node});
            scheduling_state->planContainerPrewarm(node, function);
        }

        // Set the minimum of the wake-up dates
        double earliest_wake_update = DBL_MAX;
        double now = S4U_Simulation::getClock();
        std::vector<std::shared_ptr<ServerlessSchedulingPolicy>> policies = {
            _invocation_ordering_policy,
            _node_selection_policy,
            _eviction_policy,
            _plan_selection_policy,
            _node_scaling_policy,
            _container_prewarm_policy,
            _invocation_failing_policy,
            _reusable_container_selection_policy
        };
        for (auto const& policy : policies) {
            double policy_wakeup_date = policy->getNextWakeupDate();
            if (policy_wakeup_date <= now) {
                throw std::runtime_error("A policy should always return a wake-up date in the future");
            }
            earliest_wake_update = std::min(earliest_wake_update, policy->getNextWakeupDate());
        }

        if (earliest_wake_update != DBL_MAX) {
            decisions->scheduler_wakeup_date = earliest_wake_update;
        }

        return decisions;
    }

    /**
     * @brief Select a node and an optional dispatch/preparation plan for one
     *        invocation, using the current working state.
     *
     * Candidate alternatives are evaluated without committing their resource
     * changes. The caller applies the chosen plan to the working state and
     * protects the invocation's required layers on a selected node.
     *
     * @param scheduling_state Current mutable planning snapshot.
     * @param inv Invocation being considered.
     * @return {nullptr, nullptr} if no node is selected; {node, nullptr} to wait
     *         for image preparation already in progress; or {node, decisions}
     *         for a selected dispatch/preparation plan. Selecting a waiting node
     *         does not reserve invocation capacity or a future container position.
     */
    std::pair<std::shared_ptr<ServerlessComputeNode>, std::shared_ptr<ServerlessSchedulingDecisions>>
    GreedyServerlessScheduler::pickComputeNode(
        const std::shared_ptr<ServerlessSchedulingState>& scheduling_state,
        const std::shared_ptr<Invocation>& inv) {
        std::shared_ptr<ServerlessComputeNode> picked_node;

        /* Invoke the node selection policy */
        auto selected_nodes = _node_selection_policy->selectCandidateNodes(scheduling_state, inv);

        /* Filter out all compute nodes that cannot possibly run that invocation */
        std::vector<std::shared_ptr<ServerlessComputeNode>> candidate_nodes;
        for (const auto& node : scheduling_state->getActivatedComputeNodes()) {
            if (selected_nodes.count(node) == 0) {
                continue;
            }

            if (scheduling_state->getNumPossibleNewInvocationsAtComputeNode(node) == 0) {
                continue;
            }
            candidate_nodes.push_back(node);
        }

        // Pass #1: Can we reuse an existing container?
        auto [target_node, target_idle_container] = _reusable_container_selection_policy->selectReusableContainer(
            scheduling_state,
            inv,
            candidate_nodes);
        if (target_idle_container) {
            auto decisions = std::make_shared<ServerlessSchedulingDecisions>();
            decisions->invocation_dispatches.push_back({inv, target_node, target_idle_container});
            return {target_node, decisions};
        }

        auto image = inv->getFunction()->getImage();

        // Pass #2: Can we start a new container (with some desirable eviction actions)
        std::map<std::shared_ptr<ServerlessComputeNode>, std::shared_ptr<ServerlessSchedulingDecisions>>
            invocation_plan;
        for (auto const& node : candidate_nodes) {
            if (not scheduling_state->areAllImageLayersOnDisk(image, node)) continue;
            if (not scheduling_state->areAllImageLayersInRAM(image, node)) continue;

            auto decisions = std::make_shared<ServerlessSchedulingDecisions>();
            decisions->invocation_dispatches.push_back({inv, node, nullptr});

            std::shared_ptr<ServerlessSchedulingDecisions> eviction_plan = makeEvictionPlan(node,
                scheduling_state,
                inv->getFunction()->getRAMSpaceLimit(),
                inv->getFunction()->getDiskSpaceLimit(),
                1,
                image->getLayers());

            if (not eviction_plan) {
                continue;
            }

            const auto available_container_positions = scheduling_state->getNumPossibleNewContainersAtComputeNode(node);

            if (available_container_positions == 0 && eviction_plan->idle_container_terminations.empty()) {
                continue;
            }

            decisions->idle_container_terminations = eviction_plan->idle_container_terminations;
            decisions->layer_evictions_from_ram = eviction_plan->layer_evictions_from_ram;
            decisions->layer_evictions_from_disk = eviction_plan->layer_evictions_from_disk;
            invocation_plan.emplace(node, decisions);
        }

        // Second, pick the node with the best plan
        if (not invocation_plan.empty()) {
            picked_node = _plan_selection_policy->pickBestInvocationPlan(invocation_plan);
            if (picked_node) {
                return {picked_node, invocation_plan.at(picked_node)};
            }
        }

        /* At this point, we cannot start a container right now, even with evictions, but we can look at loading layers in RAM */

        // Pass #3: Are all layers on their way to RAM, then optimistically reserve a slot that should be used in the next round */
        for (auto const& node : candidate_nodes) {
            const bool waiting_for_ram =
                not scheduling_state->areAllImageLayersInRAM(image, node) &&
                scheduling_state->areAllImageLayersInRAMOrOnTheirWayToRAM(image, node);
            if (not waiting_for_ram) continue;
            return {node, nullptr};
        }

        // Pass #4: Can we just trigger ALL necessary layer loads (with some desirable eviction actions)
        std::map<std::shared_ptr<ServerlessComputeNode>, std::shared_ptr<ServerlessSchedulingDecisions>> load_plan;
        for (auto const& node : candidate_nodes) {
            if (not scheduling_state->areAllImageLayersOnDisk(image, node)) continue;

            auto decisions = std::make_shared<ServerlessSchedulingDecisions>();
            sg_size_t ram_needed = 0;
            for (auto const& layer : image->getLayers()) {
                if (scheduling_state->getImageLayersInRAM(node).count(layer)) continue;
                if (scheduling_state->getImageLayersOnTheirWayToRAM(node).count(layer)) continue;
                ram_needed += layer->getRAMFootprint();
                decisions->image_layer_loads_to_RAM.push_back({layer, node});
            }
            if (decisions->image_layer_loads_to_RAM.empty()) {
                continue;
            }
            std::shared_ptr<ServerlessSchedulingDecisions> eviction_plan = makeEvictionPlan(node,
                scheduling_state,
                ram_needed,
                0,
                0,
                image->getLayers());
            if (not eviction_plan) {
                continue;
            }
            decisions->idle_container_terminations = eviction_plan->idle_container_terminations;
            decisions->layer_evictions_from_ram = eviction_plan->layer_evictions_from_ram;
            decisions->layer_evictions_from_disk = eviction_plan->layer_evictions_from_disk;
            load_plan.emplace(node, decisions);
        }
        // Second, pick the node with the best plan
        if (not load_plan.empty()) {
            picked_node = _plan_selection_policy->pickBestLayerLoadPlan(load_plan);
            if (picked_node) {
                return {picked_node, load_plan.at(picked_node)};
            }
        }

        /* At this point, we cannot trigger a layer load, so perhaps look at disk copies */

        // Pass #5: Are all layers on their way to disk, then reserve a slot that should be used in the next round */
        for (auto const& node : candidate_nodes) {
            const bool waiting_for_disk =
                not scheduling_state->areAllImageLayersOnDisk(image, node) &&
                scheduling_state->areAllImageLayersOnDiskOrOnTheirWayToDisk(image, node);
            if (not waiting_for_disk) continue;

            // Claim one slot for this scheduling round while disk preparation
            // is underway. Otherwise, arbitrarily many pending invocations can
            // wait on this node, delaying preparation on additional nodes.
            return {node, nullptr};
        }

        // Pass 6: Can we just trigger ALL layer copies (with some desirable eviction actions)
        std::map<std::shared_ptr<ServerlessComputeNode>, std::shared_ptr<ServerlessSchedulingDecisions>> staging_plan;
        for (auto const& node : candidate_nodes) {
            if (scheduling_state->areAllImageLayersOnDisk(image, node)) continue;

            auto decisions = std::make_shared<ServerlessSchedulingDecisions>();
            sg_size_t disk_needed = 0;
            for (auto const& layer : image->getLayers()) {
                if (scheduling_state->getImageLayersOnDisk(node).count(layer)) continue;
                if (scheduling_state->getImageLayersOnTheirWayToDisk(node).count(layer)) continue;
                disk_needed += layer->getDiskFootprint();
                decisions->image_layer_stagings_to_disk.push_back({layer, node}); // tentatively
            }
            if (decisions->image_layer_stagings_to_disk.empty()) {
                throw std::runtime_error("Pass #6: This should not have happened");
            }
            std::shared_ptr<ServerlessSchedulingDecisions> eviction_plan = makeEvictionPlan(node,
                scheduling_state,
                0,
                disk_needed,
                0,
                image->getLayers());
            if (not eviction_plan) {
                continue;
            }
            decisions->idle_container_terminations = eviction_plan->idle_container_terminations;
            decisions->layer_evictions_from_ram = eviction_plan->layer_evictions_from_ram;
            decisions->layer_evictions_from_disk = eviction_plan->layer_evictions_from_disk;
            staging_plan.emplace(node, decisions);
        }
        // Second, pick the node with the best plan
        if (not staging_plan.empty()) {
            picked_node = _plan_selection_policy->pickBestLayerStagingPlan(staging_plan);
            if (picked_node) {
                return {picked_node, staging_plan.at(picked_node)};
            }
        }

        /* At this point, we couldn't find anything doable for this invocation */
        return {nullptr, nullptr};
    }

    /**
     * @brief Function to make the eviction plan using the EVICTION_AVERSE policy
     * @param node The node on which evictions are to happen
     * @param scheduling_state The current scheduling state (with protected layers)
     * @param needed_ram The RAM needed
     * @param needed_disk The disk needed
     * @param num_needed_containers The number of needed containers
     * @param invocation_layers_to_protect The invocation's layer (that should be protected)
     * @return A set of eviction scheduling decisions (or nullptr)
     */
    std::shared_ptr<ServerlessSchedulingDecisions> GreedyServerlessScheduler::makeEvictionPlan(
        const std::shared_ptr<ServerlessComputeNode>& node,
        const std::shared_ptr<ServerlessSchedulingState>& scheduling_state,
        sg_size_t needed_ram,
        sg_size_t needed_disk,
        unsigned long num_needed_containers,
        const std::set<std::shared_ptr<ImageLayer>>& invocation_layers_to_protect) const {
        auto eviction_decisions = std::make_shared<ServerlessSchedulingDecisions>();

        // Compute what's really needed
        const auto available_ram = scheduling_state->getRAMSpaceAvailable(node);
        const auto available_disk = scheduling_state->getDiskSpaceAvailable(node);
        needed_ram = needed_ram > available_ram ? needed_ram - available_ram : 0;
        needed_disk = needed_disk > available_disk ? needed_disk - available_disk : 0;

        const auto available_container_positions = scheduling_state->getNumPossibleNewContainersAtComputeNode(node);

        const auto minimum_num_containers_to_evict =
            num_needed_containers > available_container_positions
                ? num_needed_containers - available_container_positions
                : 0UL;

        // Nothing needs to be reclaimed.
        if (needed_ram == 0 &&
            needed_disk == 0 &&
            minimum_num_containers_to_evict == 0) {
            return eviction_decisions;
        }

        // This method implements a completely greedy algorithm
        //   1st Evict idle containers while possible
        //   2nd Evict RAM layers while possible
        //   3rd Evict disk layers while possible

        /**************************************************/
        /** Try to evict idle containers to create space **/
        /**************************************************/

        {
            // Pick candidate to evict
            auto to_evict =
                _eviction_policy->pickIdleContainersForEviction(
                    scheduling_state->getIdleContainersAtComputeNode(node),
                    needed_ram,
                    needed_disk,
                    minimum_num_containers_to_evict);

            // Layer eviction cannot compensate for too few container positions.
            if (to_evict.size() < minimum_num_containers_to_evict) {
                return nullptr;
            }

            for (const auto& idle_container : to_evict) {
                eviction_decisions->idle_container_terminations.push_back(
                    {idle_container});
                needed_ram -= std::min(needed_ram, idle_container->getFunction()->getRAMSpaceLimit());
                needed_disk -= std::min(needed_disk, idle_container->getFunction()->getDiskSpaceLimit());
            }
        }

        /**************************************************/
        /** Try to evict layers to create space          **/
        /**************************************************/

        /** Build a list of non-evictable layers **/
        std::set<std::shared_ptr<ImageLayer>> non_evictable_layers;

        // Protected (local and global) layers
        non_evictable_layers.insert(invocation_layers_to_protect.begin(), invocation_layers_to_protect.end());
        non_evictable_layers.insert(scheduling_state->getProtectedImageLayers(node).begin(), scheduling_state->getProtectedImageLayers(node).end());

        // Layers used by initializing containers cannot be evicted
        for (auto& container : scheduling_state->getInitializingContainers(node)) {
            const auto& layers = container->getFunction()->getImage()->getLayers();
            non_evictable_layers.insert(layers.begin(), layers.end());
        }

        // Layers used by initialized containers that are not marked for eviction cannot be evicted
        for (auto& container : scheduling_state->getInitializedContainers(node)) {
            bool is_container_to_be_evicted = false;
            for (auto const& [container_to_be_evicted] : eviction_decisions->idle_container_terminations) {
                if (container_to_be_evicted == container) {
                    is_container_to_be_evicted = true;
                    break;
                }
            }
            if (is_container_to_be_evicted) {
                continue;
            }
            const auto& layers = container->getFunction()->getImage()->getLayers();
            non_evictable_layers.insert(layers.begin(), layers.end());
        }

        /** Try to evict layers from RAM to create space **/
        {
            // Build a list of candidate layers for eviction
            std::set<std::shared_ptr<ImageLayer>> candidate_victims;
            for (auto const& candidate : scheduling_state->getImageLayersInRAM(node)) {
                if (non_evictable_layers.count(candidate) > 0) {
                    continue;
                }

                // Can evicting the container even help?
                auto candidate_ram_space = candidate->getRAMFootprint();
                if ((not needed_ram) or (not candidate_ram_space)) {
                    continue;
                }
                candidate_victims.insert(candidate);
            }

            // Pick candidates to evict
            auto to_evict =
                _eviction_policy->pickImageLayersForRAMEviction(node, candidate_victims, needed_ram, needed_disk, 0);

            // Encode scheduling decisions
            for (auto const& victim : to_evict) {
                // Mark this layer for eviction
                eviction_decisions->layer_evictions_from_ram.push_back({victim, node});
                needed_ram -= std::min(needed_ram, victim->getRAMFootprint());
            }
        }

        /** Try to evict layers from disk to create space **/
        {
            // Build a list of candidate layers for eviction
            std::set<std::shared_ptr<ImageLayer>> candidate_victims;
            for (auto const& candidate : scheduling_state->getImageLayersOnDisk(node)) {
                if (non_evictable_layers.count(candidate) > 0) {
                    continue;
                }

                // Can evicting the container even help?
                auto candidate_disk_space = candidate->getDiskFootprint();
                if ((not needed_disk) or (not candidate_disk_space)) {
                    continue;
                }
                candidate_victims.insert(candidate);
            }

            // Pick candidates to evict
            auto to_evict =
                _eviction_policy->pickImageLayersForDiskEviction(node, candidate_victims, needed_ram, needed_disk, 0);

            // Encode scheduling decisions
            for (auto const& victim : to_evict) {
                // Mark this layer for eviction
                eviction_decisions->layer_evictions_from_disk.push_back({victim, node});
                needed_disk -= std::min(needed_disk, victim->getDiskFootprint());

                // If the layer is also in RAM, and not marked from eviction from RAM, then we must evict it
                // (can't have a layer in RAM and not on disk)
                auto victim_ram_space = victim->getRAMFootprint();
                if (scheduling_state->getImageLayersInRAM(node).count(victim)) {
                    bool marked_for_eviction_from_ram = false;
                    for (auto const& [layer, _] : eviction_decisions->layer_evictions_from_ram) {
                        if (layer == victim) {
                            marked_for_eviction_from_ram = true;
                            break;
                        }
                    }
                    if (not marked_for_eviction_from_ram) {
                        eviction_decisions->layer_evictions_from_ram.push_back({victim, node});
                        needed_ram -= std::min(needed_ram, victim_ram_space);
                    }
                }
            }
        }

        // Container-position feasibility was checked after choosing
        // the container victims. All remaining byte deficits must be met.
        if (needed_ram != 0 || needed_disk != 0) {
            return nullptr;
        }

        return eviction_decisions;
    }
} // namespace wrench
