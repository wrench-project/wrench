/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** TO WRITE 
 **
 ** Example invocation of the simulator for a 10-task workflow, with only WMS logging:
 **    ./virtualized_cluster-bag-of-tasks-simulator 10 ./four_hosts.xml --log=custom_wms.threshold=info
 **
 ** Example invocation of the simulator for a 6-task workflow with full logging:
 **    ./virtualized_cluster-bag-of-tasks-simulator 6 ./four_hosts.xml --wrench-full-log
 **/


#include <iostream>
#include <wrench.h>

#include "HadoopMRJobWMS.h" // WMS implementation

/**
 * @brief The Simulator's main function
 *
 * @param argc: argument count
 * @param argv: argument array
 * @return 0 on success, non-zero otherwise
 */
int main(int argc, char **argv) {
    /* Declare a WRENCH simulation object */
    wrench::Simulation simulation;

    /* Initialize the simulation, which may entail extracting WRENCH-specific and
     * Simgrid-specific command-line arguments that can modify general simulation behavior.
     * Two special command-line arguments are --help-wrench and --help-simgrid, which print
     * details about available command-line arguments. */
    simulation.init(&argc, argv);

    /* Parsing of the command-line arguments for this WRENCH simulation */
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <xml platform file> [--log=custom_wms.threshold=info]" << std::endl;
        exit(1);
    }

    /* Reading and parsing the platform description file, written in XML following the SimGrid-defined DTD,
     * to instantiate the simulated platform */
    std::cerr << "Instantiating simulated platform..." << std::endl;
    simulation.instantiatePlatform(argv[1]);

    /* Declare an (empty) workflow */
    wrench::Workflow workflow;

    /* Instantiate a WMS, to be stated on WMSHost, which is responsible
     * for executing the workflow. */

    auto wms = simulation.add(
            new wrench::HadoopMRJobWMS("WMSHost"));

    /* Associate the workflow to the WMS */
    wms->addWorkflow(&workflow);

    /* Launch the simulation. This call only returns when the simulation is complete. */
    std::cerr << "Launching the Simulation..." << std::endl;
    try {
        simulation.launch();
    } catch (std::runtime_error &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    std::cerr << "Simulation done!" << std::endl;

    return 0;
}
