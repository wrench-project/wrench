
/**
 * Copyright (c) 2017-2023. The WRENCH Team.
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
 **    ./wrench-example-storage-service-proxy-basic ./proxy_platform.xml
 **
 ** Example invocation of the simulator with simulation controller logging
 **    ./wrench-example-storage-service-proxy-basic ./proxy_platform.xml
 **/

#include <iostream>
#include <wrench-dev.h>

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

    /* Parsing of the command-line arguments */
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <xml platform file>  [--log=controller.threshold=info | --wrench-full-log]" << std::endl;
        exit(1);
    }

    /* Instantiating the simulated platform */
    simulation->instantiatePlatform(argv[1]);

    /* Instantiate a bare-metal compute service on the platform */
    auto baremetal_service = simulation->add(new wrench::BareMetalComputeService("Client", {"Client"}, "", {}, {}));
    simulation->add(baremetal_service);

    /* Creat a proxy configure with two storage services (a "remote" and a "cache") */
    auto remote = simulation->add(wrench::SimpleStorageService::createSimpleStorageService("Remote", {"/"}));
    auto cache = simulation->add(
            wrench::SimpleStorageService::createSimpleStorageService(
                    "Proxy", /* The cache doesn't HAVE to be on the proxy, but it really should be otherwise simulation may be inaccurate */
                    {"/"},
                    {{wrench::SimpleStorageServiceProperty::CACHING_BEHAVIOR, "LRU"}}
                    /* Make the Cache an LRU cache, this is optional, but probably the desired behavior */
                    ));
    auto proxy = simulation->add(
            wrench::StorageServiceProxy::createRedirectProxy(
                    "Proxy",
                    cache,
                    remote, /*optional set remote as the default remote server */
                    {{wrench::StorageServiceProxyProperty::UNCACHED_READ_METHOD, "CopyThenRead"}}
                    /* There are 3 read methods, CopyThenRead, magicRead, and readThrough, check the StorageServiceProxyPropery docs for more details*/
                    ));

    /* Create another storage service, the "target" */
    auto target = simulation->add(wrench::SimpleStorageService::createSimpleStorageService("Target", {"/"}));

    /* Instantiate an execution controller */
    auto controller = simulation->add(new wrench::Controller(baremetal_service, proxy, remote, target, "Client"));

    /* Launch the simulation */
    std::cerr << "Launching the Simulation...\n";
    simulation->launch();
    std::cerr << "Simulation done!\n";

    return 0;
}
