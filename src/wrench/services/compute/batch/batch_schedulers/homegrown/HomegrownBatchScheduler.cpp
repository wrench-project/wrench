/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/compute/batch/BatchComputeService.h>
#include <wrench/services/compute/batch/batch_schedulers/homegrown/HomegrownBatchScheduler.h>

namespace wrench {

    /**
     * @brief First-Fit selection of hosts
     * @param cs: the compute service
     * @param num_nodes: number of nodes
     * @param cores_per_node: number of cores per node
     * @param ram_per_node: RAM per node in bytes
     */
    std::map<simgrid::s4u::Host *, std::tuple<unsigned long, double>> HomegrownBatchScheduler::selectHostsFirstFit(BatchComputeService *cs,
                                                                                                                   unsigned long num_nodes,
                                                                                                                   unsigned long cores_per_node,
                                                                                                                   double ram_per_node) {

        std::map<simgrid::s4u::Host *, std::tuple<unsigned long, double>> resources;
        unsigned long host_count = 0;
        for (auto &available_nodes_to_core: cs->available_nodes_to_cores) {
            if (available_nodes_to_core.second >= cores_per_node) {
                //Remove that many cores from the available_nodes_to_core
                //                available_nodes_to_core.second -= cores_per_node;
                resources[available_nodes_to_core.first] = std::make_tuple(cores_per_node, ram_per_node);
                if (++host_count >= num_nodes) {
                    break;
                }
            }
        }
        if (resources.size() < num_nodes) {
            resources = {};
        } else {
            for (auto const &h: resources) {
                cs->available_nodes_to_cores[h.first] -= std::get<0>(h.second);
            }
        }
        return resources;
    }

    /**
     * @brief Best-Fit host selection
     * @param cs: the compute service
     * @param num_nodes: number of nodes
     * @param cores_per_node: number of cores per node
     * @param ram_per_node: RAM per node in bytes
     */
    std::map<simgrid::s4u::Host *, std::tuple<unsigned long, double>> HomegrownBatchScheduler::selectHostsBestFit(BatchComputeService *cs,
                                                                                                                  unsigned long num_nodes,
                                                                                                                  unsigned long cores_per_node,
                                                                                                                  double ram_per_node) {

        std::map<simgrid::s4u::Host *, std::tuple<unsigned long, double>> resources;

        while (resources.size() < num_nodes) {
            unsigned long target_slack = 0;
            simgrid::s4u::Host *target_host = nullptr;
            unsigned long target_num_cores = 0;

            for (auto h: cs->available_nodes_to_cores) {
                auto host = std::get<0>(h);
                unsigned long num_available_cores = std::get<1>(h);
                if (num_available_cores < cores_per_node) {
                    continue;
                }
                unsigned long tentative_target_num_cores = std::min(num_available_cores, cores_per_node);
                unsigned long tentative_target_slack =
                        num_available_cores - tentative_target_num_cores;

                if (target_host == nullptr ||
                    (tentative_target_num_cores > target_num_cores) ||
                    ((tentative_target_num_cores == target_num_cores) &&
                     (target_slack > tentative_target_slack))) {
                    target_host = host;
                    target_num_cores = tentative_target_num_cores;
                    target_slack = tentative_target_slack;
                }
            }
            if (target_host == nullptr) {
                break;
            }
            cs->available_nodes_to_cores[target_host] -= cores_per_node;
            resources[target_host] = std::make_tuple(cores_per_node, ComputeService::ALL_RAM);
        }

        if (resources.size() < num_nodes) {
            for (auto const &h: resources) {
                cs->available_nodes_to_cores[h.first] += std::get<0>(h.second);
            }
            return {};
        } else {
            return resources;
        }
    }

    /**
     * @brief Round-Robin selection of hosts
     * @param cs: the compute service
     * @param round_robin_host_selector_idx: current host selector index
     * @param num_nodes: number of nodes
     * @param cores_per_node: number of cores per node
     * @param ram_per_node: RAM per node in bytes
     */
    std::map<simgrid::s4u::Host *, std::tuple<unsigned long, double>> HomegrownBatchScheduler::selectHostsRoundRobin(BatchComputeService *cs,
                                                                                                                     unsigned long *round_robin_host_selector_idx,
                                                                                                                     unsigned long num_nodes,
                                                                                                                     unsigned long cores_per_node,
                                                                                                                     double ram_per_node) {
        std::map<simgrid::s4u::Host *, std::tuple<unsigned long, double>> resources;
        unsigned long cur_host_idx = *round_robin_host_selector_idx;
        unsigned long host_count = 0;
        do {
            cur_host_idx = (cur_host_idx + 1) % cs->available_nodes_to_cores.size();
            auto it = cs->compute_hosts.begin();
            it = it + cur_host_idx;
            auto cur_host = *it;
            unsigned long num_available_cores = cs->available_nodes_to_cores[cur_host];
            if (num_available_cores >= cores_per_node) {
                cs->available_nodes_to_cores[cur_host] -= cores_per_node;
                resources.insert(std::make_pair(cur_host, std::make_tuple(cores_per_node, ram_per_node)));
                if (++host_count >= num_nodes) {
                    break;
                }
            }
        } while (cur_host_idx != *round_robin_host_selector_idx);

        if (resources.size() < num_nodes) {
            for (auto const &h: resources) {
                cs->available_nodes_to_cores[h.first] += cores_per_node;
            }
            return {};
        } else {
            *round_robin_host_selector_idx = cur_host_idx;
            return resources;
        }
    }

}// namespace wrench
