/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_SIMULATION_H
#define WRENCH_SIMULATION_H

#include <string>
#include <vector>

#include "wrench/services/file_registry/FileRegistryService.h"
#include "wrench/services/network_proximity/NetworkProximityService.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/simulation/SimulationOutput.h"
#include "wrench/wms/WMS.h"
#include "wrench/workflow/job/StandardJob.h"


namespace wrench {

    class StorageService;

    /**
     * @brief The simulation state
     */
    class Simulation {

    public:
        Simulation();

        ~Simulation();

        void init(int *, char **);

        void instantiatePlatform(std::string);

        std::vector<std::string> getHostnameList();
        std::map<std::string, std::vector<std::string>> getHostnameListByCluster();

        bool hostExists(std::string hostname);

        void launch();

        ComputeService * add(ComputeService *);
        StorageService * add(StorageService *);
        NetworkProximityService * add(NetworkProximityService *);
        WMS * add(WMS *);
        FileRegistryService * add(FileRegistryService *);

        void stageFile(WorkflowFile *file, StorageService *storage_service);

        void stageFiles(std::map<std::string, WorkflowFile *> files, StorageService *storage_service);

        SimulationOutput getOutput();

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/


        double getCurrentSimulatedDate();

        static double getHostMemoryCapacity(std::string hostname);

        static unsigned long getHostNumCores(std::string hostname);

        static double getHostFlopRate(std::string hostname);

        static double getMemoryCapacity();

        static void sleep(double duration);

        /***********************/
        /** \endcond            */
        /***********************/

    private:
        SimulationOutput output;

        std::unique_ptr<S4U_Simulation> s4u_simulation;

        std::set<std::shared_ptr<WMS>> wmses;

        std::set<std::shared_ptr<FileRegistryService>> file_registry_services;

        std::set<std::shared_ptr<NetworkProximityService>> network_proximity_services;

        std::set<std::shared_ptr<ComputeService>> compute_services;

        std::set<std::shared_ptr<StorageService>> storage_services;

        void checkSimulationSetup();

        void startAllProcesses();

    };

};

#endif //WRENCH_SIMULATION_H
