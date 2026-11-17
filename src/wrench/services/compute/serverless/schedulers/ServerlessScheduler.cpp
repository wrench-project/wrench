#include "wrench/services/compute/serverless/schedulers/ServerlessScheduler.h"
#include "wrench/logging/TerminalOutput.h"

WRENCH_LOG_CATEGORY(wrench_serverless_scheduler, "Log category for Serverless Scheduler");

namespace wrench {

    /**
     * @brief Method to print scheduling decisions
     */
    void ServerlessSchedulingDecisions::log() {
        if (compute_node_activations.empty() and
            compute_node_deactivations.empty() and
            invocation_fails.empty() and
            idle_container_terminations.empty() and
            layer_evictions_from_ram.empty() and
            layer_evictions_from_disk.empty() and
            image_layer_stagings_to_disk.empty() and
            image_layer_loads_to_RAM.empty() and
            invocation_dispatches.empty() and
            container_prewarms.empty() and
            not scheduler_wakeup_date.has_value()) {
            WRENCH_INFO("** GREEDY SERVERLESS SCHEDULING: NO SCHEDULING DECISIONS **");
            return;
        }

        WRENCH_INFO("** GREEDY SERVERLESS SCHEDULING DECISIONS **");
        for (const auto& [num_compute_nodes] : compute_node_activations) {
            WRENCH_INFO("  Compute node activations: %lu compute nodes to activate", num_compute_nodes);
        }
        for (const auto& [node] : compute_node_deactivations) {
            WRENCH_INFO("  Compute node deactivation: %s compute nodes to deactivate", node->hostname.c_str());
        }
        for (const auto& [invocation] : invocation_fails) {
            WRENCH_INFO("  Forceful invocation fail: for function %s", invocation->getFunction()->getName().c_str());
        }
        for (const auto& [container] : idle_container_terminations) {
            WRENCH_INFO("  Idle container termination: for function %s at %s",
                container->getFunction()->getName().c_str(),
                container->getComputeNode()->hostname.c_str());
        }
        for (const auto& [layer, node] : layer_evictions_from_ram) {
            WRENCH_INFO("  Layer eviction from RAM: layer %s at %s", layer->getName().c_str(), node->hostname.c_str());
        }
        for (const auto& [layer, node] : layer_evictions_from_disk) {
            WRENCH_INFO("  Layer eviction from disk: layer %s at %s", layer->getName().c_str(), node->hostname.c_str());
        }
        for (const auto& [layer, node] : image_layer_stagings_to_disk) {
            WRENCH_INFO("  Image layer staging: layer %s at %s", layer->getName().c_str(), node->hostname.c_str());
        }
        for (const auto& [layer, node] : image_layer_loads_to_RAM) {
            WRENCH_INFO("  Image layer load: layer %s at %s", layer->getName().c_str(), node->hostname.c_str());;
        }
        for (const auto& [invocation, node, container] : invocation_dispatches) {
            WRENCH_INFO("  Invocation dispatch: for %s at %s (%s)", invocation->getFunction()->getName().c_str(),
                node->hostname.c_str(),
                (container ? "on an existing container" : "on a new container"));
        }
        for (const auto& [function, node] : container_prewarms) {
            WRENCH_INFO("  Container prewarm: for %s at %s", function->getName().c_str(), node->hostname.c_str());
        }
        if (scheduler_wakeup_date.has_value()) {
            WRENCH_INFO("  Scheduler wakeup requested at: %.2lf", *scheduler_wakeup_date);
        }
    }
}
