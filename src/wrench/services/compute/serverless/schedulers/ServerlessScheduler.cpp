#include <wrench/services/compute/serverless/schedulers/ServerlessScheduler.h>
#include <wrench/logging/TerminalOutput.h>

WRENCH_LOG_CATEGORY(wrench_serverless_scheduler, "Log category for Serverless Scheduler");

namespace wrench {

    /**
     * @brief Method to print scheduling decisions
     */
    void ServerlessSchedulingDecisions::print() {
        if (compute_node_activations.empty() and
            compute_node_deactivations.empty() and
            invocation_fails.empty() and
            idle_container_terminations.empty() and
            layer_evictions_from_ram.empty() and
            layer_evictions_from_disk.empty() and
            image_layer_copies_to_disk.empty() and
            image_layer_loads_to_RAM.empty() and
            invocation_dispatches.empty() and
            container_prewarms.empty() and
            not scheduler_wakeup_date.has_value()) {
            std::cerr << "** NO SCHEDULING DECISIONS **" << std::endl;
            return;
        }

        std::cerr << "** SCHEDULING DECISIONS **" << std::endl;
        for (const auto& [num_compute_nodes] : compute_node_activations) {
            std::cerr << "  Compute node activations: " << num_compute_nodes <<  " compute nodes activated" << std::endl;
        }
        for (const auto& [node] : compute_node_deactivations) {
            std::cerr << "  Compute node deactivation: " << node->hostname << std::endl;
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
                " at " << node->hostname << " (" << (container ? "on an existing container" : "on a new container")
                << ")" << std::endl;
        }
        for (const auto& [function, node] : container_prewarms) {
            std::cerr << "  Container prewarm: for " << function->getName() <<
                " at " << node->hostname << std::endl;
        }
        if (scheduler_wakeup_date.has_value()) {
            std::cerr << "  Scheduler wakeup requested at: " << *scheduler_wakeup_date << std::endl;
        }
    }
}
