#include <wrench.h>


#include <utility>
#include <algorithm>
#include <memory>
#include <set>
#include <vector>

#include <wrench/services/compute/serverless/schedulers/greedy_scheduler/GreedyServerlessScheduler.h>
#include <wrench/services/compute/serverless/schedulers/ServerlessSchedulingState.h>
#include <wrench/logging/TerminalOutput.h>


WRENCH_LOG_CATEGORY(wrench_greedy_serverless_scheduler, "Log category for GreedyServerlessScheduler");

namespace wrench {
    /* Anonymous namespace to implement a templated helper for the LRU and FEWEST eviction policy */
    namespace {
        /* LRU */
        // Candidates must already be eligible for eviction.
        // The two sizes are REMAINING deficits, not total allocation requests.
        // Select an oldest-first prefix and reduce the deficits in place.
        template <typename T, typename GetRAM, typename GetDisk,
                  typename GetLastAccess, typename GetId>
        std::set<std::shared_ptr<T>> pickVictimsLRU(
            const std::set<std::shared_ptr<T>>& candidates,
            sg_size_t& ram_space_to_free_up,
            sg_size_t& disk_space_to_free_up,
            GetRAM get_ram,
            GetDisk get_disk,
            GetLastAccess get_last_access,
            GetId get_id) {
            std::set<std::shared_ptr<T>> to_evict;

            if (candidates.empty() ||
                (ram_space_to_free_up == 0 && disk_space_to_free_up == 0)) {
                return to_evict;
            }

            struct Candidate {
                std::shared_ptr<T> victim;
                double last_access_date;
            };

            // Snapshot each timestamp once: the lookup may be nontrivial.
            std::vector<Candidate> ordered;
            ordered.reserve(candidates.size());

            for (const auto& candidate : candidates) {
                if (!candidate) {
                    throw std::invalid_argument("LRU candidate is null");
                }

                const double date = get_last_access(candidate);
                if (std::isnan(date)) {
                    throw std::invalid_argument("LRU access date is NaN");
                }

                ordered.push_back({candidate, date});
            }

            std::sort(
                ordered.begin(), ordered.end(),
                [&](const Candidate& a, const Candidate& b) {
                    if (a.last_access_date != b.last_access_date) {
                        return a.last_access_date < b.last_access_date;
                    }

                    return get_id(a.victim) < get_id(b.victim);
                });

            for (const auto& candidate : ordered) {
                if (ram_space_to_free_up == 0 && disk_space_to_free_up == 0) {
                    break;
                }

                const auto& victim = candidate.victim;
                to_evict.insert(victim);

                const auto ram_freed = get_ram(victim);
                const auto disk_freed = get_disk(victim);

                ram_space_to_free_up =
                    ram_freed >= ram_space_to_free_up
                        ? 0
                        : ram_space_to_free_up - ram_freed;

                disk_space_to_free_up =
                    disk_freed >= disk_space_to_free_up
                        ? 0
                        : disk_space_to_free_up - disk_freed;
            }

            return to_evict;
        }

        /* FEWEST */
        // Inputs are already-eligible victims and REMAINING deficits.
        // Returns selected victims and reduces the deficits in place.
        // An unmet deficit is left positive if the candidates are exhausted.
        template <typename T, typename GetRAM, typename GetDisk, typename GetId>
        std::set<std::shared_ptr<T>> pickVictimsFewest(
            const std::set<std::shared_ptr<T>>& candidates,
            sg_size_t& ram_space_to_free_up,
            sg_size_t& disk_space_to_free_up,
            GetRAM get_ram,
            GetDisk get_disk,
            GetId get_id) {
            std::set<std::shared_ptr<T>> to_evict;

            if (ram_space_to_free_up == 0 && disk_space_to_free_up == 0) {
                return to_evict;
            }

            std::vector<std::shared_ptr<T>> remaining(
                candidates.begin(), candidates.end());

            // Choose the sorting resource ONCE, as in the container policy.
            const bool sort_by_ram = ram_space_to_free_up != 0;

            std::sort(
                remaining.begin(),
                remaining.end(),
                [&](const auto& a, const auto& b) {
                    const auto a_size = sort_by_ram ? get_ram(a) : get_disk(a);
                    const auto b_size = sort_by_ram ? get_ram(b) : get_disk(b);

                    if (a_size != b_size) {
                        return a_size < b_size;
                    }

                    return get_id(a) < get_id(b);
                });

            // TODO: Below is a heuristic. Dynamic programming could likely get us something better,
            // TODO: since it all resembles a knapsack, etc. totally overkill most likely.
            while (!remaining.empty() &&
                (ram_space_to_free_up != 0 || disk_space_to_free_up != 0)) {
                // Prefer the first sufficient candidate in the ascending ordering.
                // If none suffices, take the last (largest) remaining candidate.
                auto victim = remaining.end() - 1;

                for (auto it = remaining.begin(); it != remaining.end(); ++it) {
                    if (get_ram(*it) >= ram_space_to_free_up &&
                        get_disk(*it) >= disk_space_to_free_up) {
                        victim = it;
                        break;
                    }
                }

                to_evict.insert(*victim);

                const auto ram_freed = get_ram(*victim);
                const auto disk_freed = get_disk(*victim);

                ram_space_to_free_up =
                    ram_freed >= ram_space_to_free_up
                        ? 0
                        : ram_space_to_free_up - ram_freed;

                disk_space_to_free_up =
                    disk_freed >= disk_space_to_free_up
                        ? 0
                        : disk_space_to_free_up - disk_freed;

                remaining.erase(victim);
            }

            return to_evict;
        }
    } // namespace

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
        _invocation_ordering_policy = std::make_shared<FCFSServerlessInvocationOrderingPolicy>();
        _node_selection_policy = std::make_shared<SelectAllPossibleServerlessNodeSelectionPolicy>();
        _eviction_policy = std::make_shared<FewestServerlessEvictionPolicy>();
        _plan_selection_policy = std::make_shared<EvictionAverseServerlessPlanSelectionPolicy>();
        _node_scaling_policy = std::make_shared<ActivateOneWhenNoNewInvocationPossibleNodeActivationPolicy>();
        _container_prewarm_policy = std::make_shared<NeverPrewarmServerlessContainerPrewarmPolicy>();
        _invocation_failing_policy = std::make_shared<NeverFailServerlessInvocationFailingPolicy>();
        _reusable_container_selection_policy = std::make_shared<
            ExpiringLatestServerlessReusableContainerSelectionPolicy>();
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


    void GreedyServerlessScheduler::excludeNodeFromPlanning(
        const std::shared_ptr<ServerlessSchedulingState>& scheduling_state,
        const std::shared_ptr<ServerlessComputeNode>& node) {

        auto& active_nodes = scheduling_state->_activated_compute_nodes;
        const auto node_it =
            std::find(active_nodes.begin(), active_nodes.end(), node);
        if (node_it == active_nodes.end()) {
            return;
        }
        active_nodes.erase(node_it);
        scheduling_state->_num_deactivated_compute_nodes += 1;

        scheduling_state->_disk_available.erase(node);
        scheduling_state->_ram_available.erase(node);
        scheduling_state->_image_layers_on_disk.erase(node);
        scheduling_state->_image_layers_on_their_way_to_disk.erase(node);
        scheduling_state->_image_layers_in_ram.erase(node);
        scheduling_state->_image_layers_on_their_way_to_ram.erase(node);
        scheduling_state->_protected_layers.erase(node);

        scheduling_state->_initializing_containers.erase(node);
        for (const auto& container :
             scheduling_state->_initialized_containers.at(node)) {
            scheduling_state->_num_possible_new_invocations_on_container.erase(container);
        }
        scheduling_state->_initialized_containers.erase(node);

        scheduling_state->_num_possible_new_invocations_on_compute_node.erase(node);
        scheduling_state->_num_possible_new_containers_on_compute_node.erase(node);
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
     * but does not reserve invocation capacity or bind it to that node. Copies
     * and loads do not become completed residency changes during planning.
     *
     * The returned operations are intentions for the service to apply, not
     * executed actions or an atomic reservation. See the class documentation
     * for phase precedence, reusable-container capacity, and planning semantics.
     *
     * @param schedulable_invocations Pending invocations whose image layers are
     *        available at the head node; worker-side preparation may still be
     *        required. May be empty.
     * @param state Current system state from which the planning snapshot is built.
     * @return A new ServerlessSchedulingDecisions object. Its operation lists may
     *         all be empty, independently of whether a future wakeup is requested.
     */
    std::shared_ptr<ServerlessSchedulingDecisions> GreedyServerlessScheduler::schedule(
        const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
        const ServerlessStateOfTheSystem* state) {
        auto decisions = std::make_shared<ServerlessSchedulingDecisions>();

        // Create a scheduling state
        auto scheduling_state = std::make_shared<ServerlessSchedulingState>(state, schedulable_invocations);

        // Invoke the invocation failing policy
        auto to_fail = _invocation_failing_policy->failSchedulableInvocations(scheduling_state);

        // Encode the "to fail" decisions
        for (auto const& inv : to_fail) {
            decisions->invocation_fails.push_back({inv});
        }

        // Filter out the "to fail" invocations from the list of schedulable invocations
        if (!to_fail.empty()) {
            auto& pending = scheduling_state->_schedulable_invocations;

            pending.erase(
                std::remove_if(
                    pending.begin(),
                    pending.end(),
                    [&to_fail](const auto& invocation) {
                        return to_fail.count(invocation) != 0;
                    }),
                pending.end());
        }


        // Invoke the node activation policy
        auto [num_nodes_to_activate, nodes_to_deactivate] = _node_scaling_policy->makeNodeScalingPlan(scheduling_state);
        // Implement node activations
        if (num_nodes_to_activate) {
            decisions->compute_node_activations.push_back({num_nodes_to_activate});
        }

        // Implement node de-activations and update the bookkeeping
        for (auto const& node : nodes_to_deactivate) {
            decisions->compute_node_deactivations.push_back({node});
            excludeNodeFromPlanning(scheduling_state, node);
        }


        // Sort the schedulable invocations in place
        _invocation_ordering_policy->sortSchedulableInvocations(scheduling_state);

        // Go through the invocations and pick target compute node
        for (const auto& inv : scheduling_state->_schedulable_invocations) {
            // Pick a target compute node
            auto [target_node, node_decisions] = this->pickComputeNode(scheduling_state, inv);

            // If no node was found, keep going
            if (!target_node) {
                continue;
            }

            // Make the layers of this invocation protected to avoid eviction by subsequence scheduling decisions
            const auto& layers = inv->getFunction()->getImage()->getLayers();
            scheduling_state->_protected_layers.at(target_node).insert(layers.begin(), layers.end());

            // If no node-level decisions, move on (it was just a reservation)
            if (!node_decisions) {
                continue;
            }

            // At this point we have a target node and decisions. We simply go through the decisions and:
            // 1) Update the scheduling state;
            // 2) add them to our global set of decisions

            // Idle container terminations
            for (const auto& [container] : node_decisions->idle_container_terminations) {
                decisions->idle_container_terminations.push_back({container});
                scheduling_state->_ram_available.at(target_node) += container->getFunction()->getRAMSpaceLimit();
                scheduling_state->_disk_available.at(target_node) += container->getFunction()->getDiskSpaceLimit();
                scheduling_state->_num_possible_new_containers_on_compute_node.at(target_node) += 1;
                scheduling_state->_initialized_containers.at(target_node).erase(container);
                scheduling_state->_num_possible_new_invocations_on_container.erase(container);
            }

            // Layer evictions from RAM
            for (const auto& [layer, _] : node_decisions->layer_evictions_from_ram) {
                if (scheduling_state->_protected_layers.at(target_node).count(layer)) {
                    throw std::runtime_error(
                        "Eviction plan attempts to evict a protected RAM layer");
                }
                auto& resident_layers = scheduling_state->_image_layers_in_ram.at(target_node);

                if (resident_layers.erase(layer) != 1) {
                    throw std::runtime_error(
                        "Eviction plan attempts to evict a nonresident "
                        "or already-evicted RAM layer");
                }
                scheduling_state->_ram_available.at(target_node) += layer->getRAMFootprint();

                decisions->layer_evictions_from_ram.push_back({layer, target_node});
            }

            // Layer evictions from disk
            for (const auto& [layer, _] : node_decisions->layer_evictions_from_disk) {
                if (scheduling_state->_protected_layers.at(target_node).count(layer)) {
                    throw std::runtime_error(
                        "Eviction plan attempts to evict a protected disk layer");
                }
                auto& resident_layers = scheduling_state->_image_layers_on_disk.at(target_node);

                if (resident_layers.erase(layer) != 1) {
                    throw std::runtime_error(
                        "Eviction plan attempts to evict a nonresident "
                        "or already-evicted disk layer");
                }
                scheduling_state->_disk_available.at(target_node) += layer->getDiskFootprint();

                decisions->layer_evictions_from_disk.push_back({layer, target_node});
            }

            // Invocations
            for (const auto& [_ignore1, _ignore2, container] : node_decisions->invocation_dispatches) {
                decisions->invocation_dispatches.push_back({inv, target_node, container});
                if (container) {
                    scheduling_state->_num_possible_new_invocations_on_container.at(container) -= 1;
                } else {
                    // The selected plan's container terminations have already been credited.
                    scheduling_state->_num_possible_new_containers_on_compute_node.at(target_node) -= 1;
                    scheduling_state->_ram_available.at(target_node) -= inv->getFunction()->getRAMSpaceLimit();
                    scheduling_state->_disk_available.at(target_node) -= inv->getFunction()->getDiskSpaceLimit();
                }
                scheduling_state->_num_possible_new_invocations_on_compute_node.at(target_node) -= 1;
            }

            // Image layer loads
            for (const auto& [layer,_] : node_decisions->image_layer_loads_to_RAM) {
                decisions->image_layer_loads_to_RAM.push_back({layer, target_node});
                scheduling_state->_image_layers_on_their_way_to_ram.at(target_node).insert(layer);
                scheduling_state->_ram_available.at(target_node) -= layer->getRAMFootprint();
            }

            // Image layer copies
            for (const auto& [layer,_] : node_decisions->image_layer_copies_to_disk) {
                decisions->image_layer_copies_to_disk.push_back({layer, target_node});
                scheduling_state->_image_layers_on_their_way_to_disk.at(target_node).insert(layer);
                scheduling_state->_disk_available.at(target_node) -= layer->getDiskFootprint();
            }
        }

        // Prewarm containers
        auto to_prewarm = _container_prewarm_policy->prewarmContainers(scheduling_state);
        for (auto const& [function, node] : to_prewarm) {
            decisions->container_prewarms.push_back({function, node});
            scheduling_state->_num_possible_new_containers_on_compute_node.at(node) -= 1;
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
        for (const auto& node : scheduling_state->_activated_compute_nodes) {
            if (selected_nodes.count(node) == 0) {
                continue;
            }

            if (scheduling_state->_num_possible_new_invocations_on_compute_node.at(node) == 0) {
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

            const auto available_container_positions = scheduling_state->_num_possible_new_containers_on_compute_node.
                                                                         at(node);

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
                if (scheduling_state->_image_layers_in_ram.at(node).count(layer)) continue;
                if (scheduling_state->_image_layers_on_their_way_to_ram.at(node).count(layer)) continue;
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
        std::map<std::shared_ptr<ServerlessComputeNode>, std::shared_ptr<ServerlessSchedulingDecisions>> copy_plan;
        for (auto const& node : candidate_nodes) {
            if (scheduling_state->areAllImageLayersOnDisk(image, node)) continue;

            auto decisions = std::make_shared<ServerlessSchedulingDecisions>();
            sg_size_t disk_needed = 0;
            for (auto const& layer : image->getLayers()) {
                if (scheduling_state->_image_layers_on_disk.at(node).count(layer)) continue;
                if (scheduling_state->_image_layers_on_their_way_to_disk.at(node).count(layer)) continue;
                disk_needed += layer->getDiskFootprint();
                decisions->image_layer_copies_to_disk.push_back({layer, node}); // tentatively
            }
            if (decisions->image_layer_copies_to_disk.empty()) {
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
            copy_plan.emplace(node, decisions);
        }
        // Second, pick the node with the best plan
        if (not copy_plan.empty()) {
            picked_node = _plan_selection_policy->pickBestLayerCopyPlan(copy_plan);
            if (picked_node) {
                return {picked_node, copy_plan.at(picked_node)};
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
        const auto available_ram = scheduling_state->_ram_available.at(node);
        const auto available_disk = scheduling_state->_disk_available.at(node);
        needed_ram = needed_ram > available_ram ? needed_ram - available_ram : 0;
        needed_disk = needed_disk > available_disk ? needed_disk - available_disk : 0;

        const auto available_container_positions = scheduling_state->_num_possible_new_containers_on_compute_node.
                                                                     at(node);

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
            }
        }

        /**************************************************/
        /** Try to evict layers from RAM to create space **/
        /**************************************************/

        {
            // Build a list of candidate layers for eviction
            std::set<std::shared_ptr<ImageLayer>> candidate_victims;
            for (auto const& candidate : scheduling_state->_image_layers_in_ram.at(node)) {
                auto candidate_ram_space = candidate->getRAMFootprint();

                // Can evicting the container even help?
                if ((not needed_ram) or (not candidate_ram_space)) {
                    continue;
                }

                // If the layer is protected, skip it
                if (scheduling_state->_protected_layers.at(node).count(candidate)) {
                    continue;
                }
                if (invocation_layers_to_protect.count(candidate)) {
                    continue;
                }

                // Is the layer evictable in the first place?

                // Perhaps it's being used by an idle container that's not marked for eviction?
                bool is_victim_layer_evictable = true;
                for (auto& container : scheduling_state->getIdleContainersAtComputeNode(node)) {
                    bool is_container_to_be_evicted = false;
                    for (auto const& [container_to_be_evicted] : eviction_decisions->idle_container_terminations) {
                        if (container_to_be_evicted == container) {
                            is_container_to_be_evicted = true;
                            break;
                        }
                    }

                    // If the container is to be evicted, we're always fine
                    if (is_container_to_be_evicted) {
                        continue;
                    }

                    // If the container uses the layer, then the layer cannot be evicted
                    if (container->getFunction()->getImage()->getLayers().count(candidate)) {
                        is_victim_layer_evictable = false;
                        break;
                    }
                }
                if (not is_victim_layer_evictable) {
                    continue;
                }

                // Perhaps it's being used by an initializing container?
                is_victim_layer_evictable = true;
                for (auto& container : scheduling_state->_initializing_containers.at(node)) {
                    // If the container uses the layer, then the layer cannot be evicted
                    if (container->getFunction()->getImage()->getLayers().count(candidate)) {
                        is_victim_layer_evictable = false;
                        break;
                    }
                }
                // Perhaps it's being used by an initialized container?
                for (auto& container : scheduling_state->_initialized_containers.at(node)) {
                    // If the container is to be evicted, we're always fine
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
                    // If the container uses the layer, then the layer cannot be evicted
                    if (container->getFunction()->getImage()->getLayers().count(candidate)) {
                        is_victim_layer_evictable = false;
                        break;
                    }
                }
                if (not is_victim_layer_evictable) {
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
            }
        }

        /***************************************************/
        /** Try to evict layers from disk to create space **/
        /***************************************************/

        {
            // Build a list of candidate layers for eviction
            std::set<std::shared_ptr<ImageLayer>> candidate_victims;

            for (auto const& candidate : scheduling_state->_image_layers_on_disk.at(node)) {
                auto candidate_disk_space = candidate->getDiskFootprint();
                auto candidate_ram_space = candidate->getRAMFootprint();

                // Can evicting the container even help?
                if ((not needed_disk) or (not candidate_disk_space)) {
                    continue;
                }

                // If the layer is protected, skip it
                if (scheduling_state->_protected_layers.at(node).count(candidate)) {
                    continue;
                }
                if (invocation_layers_to_protect.count(candidate)) {
                    continue;
                }

                // Perhaps it's being used by an idle container that's not marked for eviction?
                bool is_victim_layer_evictable = true;
                for (auto& container : scheduling_state->getIdleContainersAtComputeNode(node)) {
                    bool is_container_to_be_evicted = false;
                    for (auto const& [container_to_be_evicted] : eviction_decisions->idle_container_terminations) {
                        if (container_to_be_evicted == container) {
                            is_container_to_be_evicted = true;
                            break;
                        }
                    }

                    // If the container is to be evicted, we're always fine
                    if (is_container_to_be_evicted) {
                        continue;
                    }

                    // If the container uses the layer, then the layer cannot be evicted
                    if (container->getFunction()->getImage()->getLayers().count(candidate)) {
                        is_victim_layer_evictable = false;
                        break;
                    }
                }
                if (not is_victim_layer_evictable) {
                    continue;
                }

                // Perhaps it's being used by a busy container that's not marked for eviction?
                is_victim_layer_evictable = true;
                for (auto& container : scheduling_state->_initializing_containers.at(node)) {
                    // If the container uses the layer, then the layer cannot be evicted
                    if (container->getFunction()->getImage()->getLayers().count(candidate)) {
                        is_victim_layer_evictable = false;
                        break;
                    }
                }
                for (auto& container : scheduling_state->_initialized_containers.at(node)) {
                    // If the container is to be evicted, we're always fine
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
                    // If the container uses the layer, then the layer cannot be evicted
                    if (container->getFunction()->getImage()->getLayers().count(candidate)) {
                        is_victim_layer_evictable = false;
                        break;
                    }
                }
                if (not is_victim_layer_evictable) {
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

                // If the layer is also in RAM, and not marked from eviction from RAM, then we must evict it
                // (can't have a layer in RAM and not on disk)
                auto victim_ram_space = victim->getRAMFootprint();
                if (scheduling_state->_image_layers_in_ram.at(node).count(victim)) {
                    bool marked_for_eviction_from_ram = false;
                    for (auto const& [layer, _] : eviction_decisions->layer_evictions_from_ram) {
                        if (layer == victim) {
                            marked_for_eviction_from_ram = true;
                            break;
                        }
                    }
                    if (not marked_for_eviction_from_ram) {
                        eviction_decisions->layer_evictions_from_ram.push_back({victim, node});
                        needed_ram = (victim_ram_space > needed_ram ? 0 : needed_ram - victim_ram_space);
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
