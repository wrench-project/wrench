/**
 * Copyright (c) 2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SIMULATION_CONTROLLER_H
#define WRENCH_SIMULATION_CONTROLLER_H

#include <wrench-dev.h>
#include <map>
#include <vector>
#include <queue>
#include <mutex>

#include <nlohmann/json.hpp>

#include "BlockingQueue.h"
#include "KeyValueStore.h"

using json = nlohmann::json;

namespace wrench {

    /**
     * @brief A class that implements the Simulation Controller as a WMS. This is the key
     * WRENCH service that controls the simulation, handling all interaction with a
     * Simulation Daemon (which itself was spawned by wrench-daemon, and which handles
     * all interaction with the simulation client).
     */
    class SimulationController : public WMS {

    public:
        explicit SimulationController(const std::string &hostname, int sleep_us);

        void stopSimulation();

        json getSimulationTime(json data);

        json getAllHostnames(json data);

        json advanceTime(json data);

        json createStandardJob(json data);

        json getSimulationEvents(json);

        json submitStandardJob(json data);

        json getStandardJobTasks(json data);

        json getTaskFlops(json data);

        json getTaskMinNumCores(json data);

        json getTaskMaxNumCores(json data);

        json getTaskMemory(json data);

        json waitForNextSimulationEvent(json data);

        json addBareMetalComputeService(json data);

        json addSimpleStorageService(json data);

        json createTask(json data);

    private:
        // Thread-safe key value stores
        KeyValueStore<std::shared_ptr<wrench::StandardJob>> job_registry;
        KeyValueStore<std::shared_ptr<ComputeService>> compute_service_registry;
        KeyValueStore<std::shared_ptr<StorageService>> storage_service_registry;

        // Thread-safe queues for the server thread and the simulation thread to communicate
        BlockingQueue<std::pair<double, std::shared_ptr<wrench::WorkflowExecutionEvent>>> event_queue;
        BlockingQueue<wrench::ComputeService *> compute_services_to_start;
        BlockingQueue<wrench::StorageService *> storage_services_to_start;
        BlockingQueue<std::pair<std::shared_ptr<StandardJob>, std::shared_ptr<ComputeService>>> submissions_to_do;

        // The two managers
        std::shared_ptr<JobManager> job_manager;
        std::shared_ptr<DataMovementManager> data_movement_manager;

        bool keep_going = true;
        double time_horizon_to_reach = 0;
        unsigned int sleep_us;

        int main() override;

        static json eventToJSON(double date, const std::shared_ptr<wrench::WorkflowExecutionEvent> &event);
    };
}

#endif // WRENCH_SIMULATION_CONTROLLER_H
