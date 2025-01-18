#include <wrench/services/compute/serverless/ServerlessComputeService.h>
#include <wrench/logging/TerminalOutput.h>

#include "wrench/simulation/Simulation.h"

WRENCH_LOG_CATEGORY(wrench_core_serverless_service, "Log category for Serverless Compute Service");

namespace wrench
{
    ServerlessComputeService::ServerlessComputeService(const std::string& hostname,
                                                       std::vector<std::string> compute_hosts,
                                                       std::string scratch_space_mount_point,
                                                       WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                                       WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE messagepayload_list) :
        ComputeService(hostname,
                       "ServelessComputeService",
                       scratch_space_mount_point) {
        _compute_hosts = compute_hosts;

    }

    /**
     * @brief Returns true if the service supports standard jobs
     * @return true or false
     */
    bool ServerlessComputeService::supportsStandardJobs() {
        return false;
    }

    /**
     * @brief Returns true if the service supports compound jobs
     * @return true or false
     */
    bool ServerlessComputeService::supportsCompoundJobs() {
        return false;
    }

    /**
     * @brief Returns true if the service supports pilot jobs
     * @return true or false
     */
    bool ServerlessComputeService::supportsPilotJobs() {
        return false;
    }

    int ServerlessComputeService::main() {

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_MAGENTA);
        WRENCH_INFO("Serverless provider starting");

        while (true) {
            Simulation::sleep(100);
        }
        return 0;
    }

};
