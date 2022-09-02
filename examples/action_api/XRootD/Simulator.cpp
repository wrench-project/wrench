
/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** This is the main function for a WRENCH simulator. The simulator takes
 ** as input an XML platform description file.
 **
 ** Example invocation of the simulator with no logging:
 **    ./wrench-example-xrootd-basic ./xrootd_platform.xml
 **/

#include <iostream>
#include <wrench-dev.h>
#include <wrench/services/storage/xrootd/XRootDDeployment.h>

#include <wrench/services/storage/xrootd/Node.h>
#include "Controller.h"


/**
 * @brief The Simulator's main function
 *
 * @param argc: argument count
 * @param argv: argument array
 * @return 0 on success, non-zero otherwise
 */
int main(int argc, char **argv) {

    /* Create a WRENCH simulation object */
    auto simulation = wrench::Simulation::createSimulation();

    /* Initialize the simulation */
    simulation->init(&argc, argv);
    bool reduced=false;

    /* Parsing of the command-line arguments */
    if (argc  != 2) {
        std::cerr << "Usage: " << argv[0] << " <xml platform file>  [--log=controller.threshold=info | --wrench-full-log]" << std::endl;
        exit(1);
    }

    /* Instantiating the simulated platform */
    simulation->instantiatePlatform(argv[1]);

    /* Instantiate a bare-metal compute service on the platform */
    auto baremetal_service = simulation->add(new wrench::BareMetalComputeService("user", {"user"}, "", {}, {}));
    simulation->add(baremetal_service);

    /* Create an XRootD manager object, with a couple of configuration properties.
     * The REDUCED_SIMULATION property, when set to "true" ("false" is the default),
     * abstracts away more simulation details, which speeds up the simulation but
     * may make it less realistic. See the documentation for more details.
     */
    wrench::XRootD::XRootDDeployment xrootd_deployment(simulation,
                                                   {
                                                 {wrench::XRootD::Property::CACHE_MAX_LIFETIME,"28800"},
                                                 {wrench::XRootD::Property::REDUCED_SIMULATION,"false"}
                                         },
                                                   {});

    /* Construct an XRootD tree as follows (vertices are host names)

            root
         /   |   \
    leaf1  leaf2  super1
                 /   |   \
            leaf3  leaf4  super2
                         /   |   \
                    leaf5  leaf6  super3
                                 /   |   \
                            leaf7  leaf8  super4
                                         /   |   \
                                    leaf9  leaf10  leaf11
    */
    auto root = xrootd_deployment.createRootSupervisor("root");

    root->addChildStorageServer("leaf1","/",{},{});
    root->addChildStorageServer("leaf2","/",{},{});
    auto super1 = root->addChildSupervisor("super1");

    super1->addChildStorageServer("leaf3","/",{},{});
    super1->addChildStorageServer("leaf4","/",{},{});
    auto super2 = super1->addChildSupervisor("super2");

    super2->addChildStorageServer("leaf5","/",{},{});
    super2->addChildStorageServer("leaf6","/",{},{});
    auto super3 = super2->addChildSupervisor("super3");

    super3->addChildStorageServer("leaf7","/",{},{});
    super3->addChildStorageServer("leaf8","/",{},{});
    auto super4 = super3->addChildSupervisor("super4");
    super4->addChildStorageServer("leaf9","/",{},{});
    super4->addChildStorageServer("leaf10","/",{},{});
    super4->addChildStorageServer("leaf11","/",{},{});

    /* Instantiate an execution controller */
    auto controller = simulation->add(new wrench::Controller(baremetal_service, &xrootd_deployment, "root"));

    /* Launch the simulation */
    simulation->launch();

    return 0;
}
