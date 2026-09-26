#include <wrench.h>


#include <utility>
#include <algorithm>
#include <memory>
#include <set>
#include <vector>

#include <wrench/services/compute/serverless/schedulers/greedy_scheduler/GreedyServerlessScheduler.h>
#include <wrench/services/compute/serverless/schedulers/ServerlessSchedulingState.h>
#include <wrench/services/compute/serverless/schedulers/greedy_scheduler/invocation_sorting_policies/ServerlessInvocationOrderingPolicy.h>
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
     * @brief Constructor
     * @param invocation_ordering the invocation sorting policy
     * @param eviction the eviction policy
     * @param plan_selection the plan selection policy
     */
    GreedyServerlessScheduler::GreedyServerlessScheduler(
            std::shared_ptr<ServerlessInvocationOrderingPolicy> invocation_ordering,
            std::shared_ptr<ServerlessEvictionPolicy> eviction,
            std::shared_ptr<ServerlessPlanSelectionPolicy> plan_selection) :
                  _invocation_ordering_policy(std::move(invocation_ordering)),
                  _eviction_policy(std::move(eviction)),
                  _plan_selection_policy(std::move(plan_selection)) {

        if (!_invocation_ordering_policy ||
            !_eviction_policy ||
            !_plan_selection_policy) {
            throw std::invalid_argument(
                "GreedyServerlessScheduler requires three non-null policies");
            }
    }

    /**
     * @brief Get the scheduler's invocation ordering policy
     * @return the policy
     */
    ServerlessInvocationOrderingPolicy* GreedyServerlessScheduler::getInvocationOrderingPolicy() const {
        return _invocation_ordering_policy.get();
    }

    /**
     * @brief Get the scheduler's eviction policy
     * @return the policy
     */
    ServerlessEvictionPolicy *GreedyServerlessScheduler::getEvictionPolicy() const {
        return _eviction_policy.get();
    }

    /**
     * @brief Get the scheduler's plan selection policy
     * @return the policy
     */
    ServerlessPlanSelectionPolicy *GreedyServerlessScheduler::getPlanSelectionPolicy() const {
        return _plan_selection_policy.get();
    }

    /**
     * @brief Given the list of schedulable invocations and the current system state, decide on which
     *        compute node each invocation should go and then encode what needs to be done to make
     *        it happen.
     *
     * @param schedulable_invocations A list of schedulable (i.e., with image on the head node's disk) invocations
     * @param state the current system state
     *
     * @return A SchedulingDecisions object
     */
    std::shared_ptr<ServerlessSchedulingDecisions> GreedyServerlessScheduler::schedule(
        const std::vector<std::shared_ptr<Invocation>>& schedulable_invocations,
        const ServerlessStateOfTheSystem* state) {
        auto decisions = std::make_shared<ServerlessSchedulingDecisions>();

        // Create a scheduling state
        auto scheduling_state = std::make_shared<ServerlessSchedulingState>(state, schedulable_invocations);

        // Sort the schedulable invocations
        auto sorted_schedulable_invocations = _invocation_ordering_policy->sortSchedulableInvocations(
            scheduling_state, schedulable_invocations);

        // Determine the total number of cores available
        unsigned int num_cores_still_available = 0;
        for (auto const& [node, count] : scheduling_state->_cores_available) {
            num_cores_still_available += count;
        }

        // Go through the invocations and pick target compute node
        for (const auto& inv : sorted_schedulable_invocations) {
            // If all cores are allocated abort
            if (num_cores_still_available == 0) break;

            // Pick a target compute node
            auto [target_node, node_decisions] = this->pickComputeNode(scheduling_state, inv);

            // If no node was found, keep going
            if (!target_node) {
                continue;
            }

            // At this point, a core is to be reserved at the target_node
            scheduling_state->_cores_available.at(target_node)--;
            num_cores_still_available--;

            // Make the layers of this invocation protected to avoid eviction by subsequence scheduling decisions
            const auto& layers = inv->getFunction()->getImage()->getLayers();
            scheduling_state->_protected_layers.at(target_node).insert(layers.begin(), layers.end());

            // If no node-level decisions, move on (it was just a reservation of a core)
            if (!node_decisions) {
                continue;
            }

            // At this point we have a target node and decisions. We simply go through the decisions and:
            // 1) Update the scheduling state;
            // 2) add them to our global set of decisions

            // Idle container terminations
            for (const auto& [container] : node_decisions->idle_container_terminations) {
                decisions->idle_container_terminations.push_back({container});
                scheduling_state->_idle_containers.at(target_node).erase(container);
                scheduling_state->_ram_available.at(target_node) += container->getFunction()->getRAMSpaceLimit();
                scheduling_state->_disk_available.at(target_node) += container->getFunction()->getDiskSpaceLimit();
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
                    scheduling_state->_idle_containers.at(target_node).erase(container);
                } else {
                    scheduling_state->_ram_available.at(target_node) -= inv->getFunction()->getRAMSpaceLimit();
                    scheduling_state->_disk_available.at(target_node) -= inv->getFunction()->getDiskSpaceLimit();
                }
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
        return decisions;
    }

    /**
     * @brief Given an invocation, pick a compute node
     * @param scheduling_state the scheduling state
     * @param inv an invocation
     * @return a compute node and a set of scheduling decisions that would need to be made for this node to be used
     *     {nullptr, nullptr}	No placement selected; do not claim a core
     *     {node, nullptr}	    Claim one core on this node for this round; wait for preparation
     *     {node, decisions}	Claim one core on this node for this round; commit the specified work
     */
    std::pair<std::shared_ptr<ServerlessComputeNode>, std::shared_ptr<ServerlessSchedulingDecisions>>
    GreedyServerlessScheduler::pickComputeNode(
        const std::shared_ptr<ServerlessSchedulingState>& scheduling_state,
        const std::shared_ptr<Invocation>& inv) {
        std::shared_ptr<ServerlessComputeNode> picked_node;

        /* First, filter out all compute nodes that have no available cores, to preserve some efficiency */
        std::vector<std::shared_ptr<ServerlessComputeNode>> not_fully_busy_compute_nodes;
        for (auto const& node : scheduling_state->_compute_nodes) {
            if (scheduling_state->_cores_available.at(node) > 0) {
                not_fully_busy_compute_nodes.push_back(node);
            }
        }


        /* Go through the compute node in multiple passes, each time lowering "standards" */

        // Pass #1: Can we reuse an existing idle container?
        for (auto const& node : not_fully_busy_compute_nodes) {
            for (auto const& idle_container : scheduling_state->_idle_containers.at(node)) {
                if (idle_container->getFunction() == inv->getFunction().get()) {
                    auto decisions = std::make_shared<ServerlessSchedulingDecisions>();
                    decisions->invocation_dispatches.push_back({inv, node, idle_container});
                    return {node, decisions};
                }
            }
        }

        auto image = inv->getFunction()->getImage();

        // Pass #2: Can we start a new container (with some desirable eviction actions)
        std::map<std::shared_ptr<ServerlessComputeNode>, std::shared_ptr<ServerlessSchedulingDecisions>>
            invocation_plan;
        for (auto const& node : not_fully_busy_compute_nodes) {
            if (not scheduling_state->areAllImageLayersOnDisk(image, node)) continue;
            if (not scheduling_state->areAllImageLayersInRAM(image, node)) continue;

            auto decisions = std::make_shared<ServerlessSchedulingDecisions>();
            decisions->invocation_dispatches.push_back({inv, node, nullptr});

            std::shared_ptr<ServerlessSchedulingDecisions> eviction_plan = makeEvictionPlan(node,
                scheduling_state,
                inv->getFunction()->getRAMSpaceLimit(),
                inv->getFunction()->getDiskSpaceLimit(),
                image->getLayers());
            if (not eviction_plan) {
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

        // Pass #3: Are all layers on their way to RAM, then optimistically reserve a core that should be used in the next round */
        for (auto const& node : not_fully_busy_compute_nodes) {
            const bool waiting_for_ram =
                not scheduling_state->areAllImageLayersInRAM(image, node) &&
                scheduling_state->areAllImageLayersInRAMOrOnTheirWayToRAM(image, node);
            if (not waiting_for_ram) continue;
            return {node, nullptr};
        }

        // Pass #4: Can we just trigger ALL necessary layer loads (with some desirable eviction actions)
        std::map<std::shared_ptr<ServerlessComputeNode>, std::shared_ptr<ServerlessSchedulingDecisions>> load_plan;
        for (auto const& node : not_fully_busy_compute_nodes) {
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

        // Pass #5: Are all layers on their way to disk, then reserve a core that should be used in the next round */
        for (auto const& node : not_fully_busy_compute_nodes) {
            const bool waiting_for_disk =
                not scheduling_state->areAllImageLayersOnDisk(image, node) &&
                scheduling_state->areAllImageLayersOnDiskOrOnTheirWayToDisk(image, node);
            if (not waiting_for_disk) continue;

            // Claim one core for this scheduling round while disk preparation
            // is underway. Otherwise, arbitrarily many pending invocations can
            // wait on this node, delaying preparation on additional nodes.
            return {node, nullptr};
        }

        // Pass 6: Can we just trigger ALL layer copies (with some desirable eviction actions)
        std::map<std::shared_ptr<ServerlessComputeNode>, std::shared_ptr<ServerlessSchedulingDecisions>> copy_plan;
        for (auto const& node : not_fully_busy_compute_nodes) {
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
     * @param invocation_layers_to_protect The invocation's layer (that should be protected)
     * @return A set of eviction scheduling decisions
     */
    std::shared_ptr<ServerlessSchedulingDecisions> GreedyServerlessScheduler::makeEvictionPlan(
        const std::shared_ptr<ServerlessComputeNode>& node,
        const std::shared_ptr<ServerlessSchedulingState>& scheduling_state,
        sg_size_t needed_ram,
        sg_size_t needed_disk,
        const std::set<std::shared_ptr<ImageLayer>>& invocation_layers_to_protect) const {
        auto eviction_decisions = std::make_shared<ServerlessSchedulingDecisions>();

        // Compute what's really needed
        const auto available_ram = scheduling_state->_ram_available.at(node);
        const auto available_disk = scheduling_state->_disk_available.at(node);
        needed_ram = needed_ram > available_ram ? needed_ram - available_ram : 0;
        needed_disk = needed_disk > available_disk ? needed_disk - available_disk : 0;

        // Are we done?
        if (needed_ram == 0 && needed_disk == 0) {
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
                _eviction_policy->pickIdleContainersForEviction(scheduling_state->_idle_containers.at(node), needed_ram,
                                                    needed_disk);

            // Encode scheduling decisions
            for (auto const& idle_container : to_evict) {
                // Mark this layer for eviction
                eviction_decisions->idle_container_terminations.push_back({idle_container});
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
                for (auto& container : scheduling_state->_idle_containers.at(node)) {
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

                // Perhaps it's being used by a busy container?
                is_victim_layer_evictable = true;
                for (auto& container : scheduling_state->_busy_containers.at(node)) {
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
                _eviction_policy->pickImageLayersForRAMEviction(node, candidate_victims, needed_ram, needed_disk);

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
                for (auto& container : scheduling_state->_idle_containers.at(node)) {
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
                for (auto& container : scheduling_state->_busy_containers.at(node)) {
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
                _eviction_policy->pickImageLayersForDiskEviction(node, candidate_victims, needed_ram, needed_disk);

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

        // The proposed reclamation must satisfy BOTH resource requirements.
        if (needed_ram != 0 || needed_disk != 0) {
            return nullptr;
        }

        return eviction_decisions;
    }

} // namespace wrench
