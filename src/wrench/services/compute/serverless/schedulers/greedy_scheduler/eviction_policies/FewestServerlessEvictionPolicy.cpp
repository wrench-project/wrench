#include <utility>
#include <algorithm>
#include <memory>
#include <set>
#include <vector>

#include <wrench/services/compute/serverless/schedulers/greedy_scheduler/eviction_policies/FewestServerlessEvictionPolicy.h>
#include <wrench/services/compute/serverless/Container.h>
#include <wrench/function/ImageLayer.h>

namespace wrench {
    /* Anonymous namespace to implement a templated helper for avoiding code duplication */
namespace {
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
    }

    /**
    * @brief Method to pick idle containers to evict using the "FEWEST" policy
    * @param idle_containers List of (idle) containers to sort
    * @param ram_space_to_free_up Amount of RAM to free up
    * @param disk_space_to_free_up Amount of disk to free up
    * @return A sorted list of (idle) containers
    */
    std::set<std::shared_ptr<Container>> FewestServerlessEvictionPolicy::pickIdleContainersForEviction(
        const std::set<std::shared_ptr<Container>>& idle_containers,
        sg_size_t& ram_space_to_free_up,
        sg_size_t& disk_space_to_free_up) {
    return pickVictimsFewest(
        idle_containers,
        ram_space_to_free_up,
        disk_space_to_free_up,
        [](const auto& container) {
            return container->getFunction()->getRAMSpaceLimit();
        },
        [](const auto& container) {
            return container->getFunction()->getDiskSpaceLimit();
        },
        [](const auto& container) {
            return container->getCreationId();
        });
}

    /**
     * @brief Method to pick layers to evict from RAM using the "Fewest" policy
     * @param node the compute node where the evictions are to take place
     * @param layers candidate victim layers
     * @param ram_space_to_free_up Amount of RAM to free up
     * @param disk_space_to_free_up Amount of disk to free up
     * @return
    */
    std::set<std::shared_ptr<ImageLayer>>
    FewestServerlessEvictionPolicy::pickImageLayersForRAMEviction(
        const std::shared_ptr<ServerlessComputeNode>& node,
        const std::set<std::shared_ptr<ImageLayer>>& layers,
        sg_size_t& ram_space_to_free_up,
        sg_size_t& disk_space_to_free_up) {
    // RAM eviction neither requires nor frees disk space.
    sg_size_t ignored_disk_deficit = 0;

    return pickVictimsFewest(
        layers,
        ram_space_to_free_up,
        ignored_disk_deficit,
        [](const auto& layer) {
            return layer->getRAMFootprint();
        },
        [](const auto&) -> sg_size_t {
            return 0;
        },
        [](const auto& layer) {
            return layer->getCreationId();
        });
}

    /**
     * @brief Method to pick layers to evict from disk using the "Fewest" policy
     * @param node the compute node where the evictions are to take place
     * @param layers candidate victim layers
     * @param ram_space_to_free_up Amount of Disk to free up
     * @param disk_space_to_free_up Amount of disk to free up
     * @return
     */
    std::set<std::shared_ptr<ImageLayer>> FewestServerlessEvictionPolicy::pickImageLayersForDiskEviction(
        const std::shared_ptr<ServerlessComputeNode>& node,
        const std::set<std::shared_ptr<ImageLayer>>& layers,
        sg_size_t& ram_space_to_free_up,
        sg_size_t& disk_space_to_free_up) {
    // Any prerequisite RAM eviction is accounted for by makeEvictionPlan().
    sg_size_t ignored_ram_deficit = 0;

    return pickVictimsFewest(
        layers,
        ignored_ram_deficit,
        disk_space_to_free_up,
        [](const auto&) -> sg_size_t {
            return 0;
        },
        [](const auto& layer) {
            return layer->getDiskFootprint();
        },
        [](const auto& layer) {
            return layer->getCreationId();
        });
}
}
