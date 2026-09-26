#include <wrench/services/compute/serverless/schedulers/greedy_scheduler/eviction_policies/LRUServerlessEvictionPolicy.h>
#include <wrench/services/compute/serverless/ServerlessComputeNode.h>
#include <wrench/services/compute/serverless/Container.h>
#include <wrench/function/ImageLayer.h>

namespace wrench {
    /* Anonymous namespace to implement a templated helper for avoiding code duplication */
    namespace {
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
    }

    /**
     * @brief Method to pick idle containers to evict using the "LRU" policy
     * @param idle_containers List of (idle) containers to sort
     * @param ram_space_to_free_up Amount of RAM to free up
     * @param disk_space_to_free_up Amount of disk to free up
     * @return A sorted list of (idle) containers
     */
    std::set<std::shared_ptr<Container>> LRUServerlessEvictionPolicy::pickIdleContainersForEviction(
        const std::set<std::shared_ptr<Container>>& idle_containers,
        sg_size_t& ram_space_to_free_up,
        sg_size_t& disk_space_to_free_up) {
        return pickVictimsLRU(
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
                return container->getIdleDate();
            },
            [](const auto& container) {
                return container->getCreationId();
            });
    }

    /**
     * @brief Method to pick layers to evict from RAM using the "LRU" policy
     * @param node the compute node where the evictions are to take place
     * @param layers candidate victim layers
     * @param ram_space_to_free_up Amount of RAM to free up
     * @param disk_space_to_free_up Amount of disk to free up
     * @return a set of layers
     */
    std::set<std::shared_ptr<ImageLayer>> LRUServerlessEvictionPolicy::pickImageLayersForRAMEviction(
        const std::shared_ptr<ServerlessComputeNode>& node,
        const std::set<std::shared_ptr<ImageLayer>>& layers,
        sg_size_t& ram_space_to_free_up,
        sg_size_t& disk_space_to_free_up) {
        sg_size_t ignored_disk_deficit = 0;

        return pickVictimsLRU(
            layers,
            ram_space_to_free_up,
            ignored_disk_deficit,
            [](const auto& layer) {
                return layer->getRAMFootprint();
            },
            [](const auto&) -> sg_size_t {
                return 0;
            },
            [node](const auto& layer) -> double {
                return node->getLayerLastAccessDateInRAM(layer);
            },
            [](const auto& layer) {
                return layer->getCreationId();
            });
    }

    /**
  * @brief Method to pick layers to evict from disk using the "LRU" policy
  * @param node the compute node where evictions take place
  * @param layers candidate victim layers
  * @param ram_space_to_free_up Amount of Disk to free up
  * @param disk_space_to_free_up Amount of disk to free up
  * @return a set of layers
  */
    std::set<std::shared_ptr<ImageLayer>> LRUServerlessEvictionPolicy::pickImageLayersForDiskEviction(
        const std::shared_ptr<ServerlessComputeNode>& node,
        const std::set<std::shared_ptr<ImageLayer>>& layers,
        sg_size_t& ram_space_to_free_up,
        sg_size_t& disk_space_to_free_up) {
        sg_size_t ignored_ram_deficit = 0;

        return pickVictimsLRU(
            layers,
            ignored_ram_deficit,
            disk_space_to_free_up,
            [](const auto&) -> sg_size_t {
                return 0;
            },
            [](const auto& layer) {
                return layer->getDiskFootprint();
            },
            [node](const auto& layer) -> double {
                return node->getLayerLastAccessDateOnDisk(layer);
            },
            [](const auto& layer) {
                return layer->getCreationId();
            });
    }
}
