
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
 ** a input an XML platform description file. It generates a workflow with
 ** a simple diamond structure, instantiates a few services on the platform, and
 ** starts an execution controller to execute the workflow using these services
 ** using a simple greedy algorithm.
 **/

#include <iostream>
#include <wrench-dev.h>
#include <wrench/services/storage/xrootd/XRootD.h>

#include <wrench/services/storage/xrootd/Node.h>
#include "../include/Controller.h"
std::string to_string(bool a){
	if(a){
		return "true";
	}else{
		return "false";
	}
}

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
    if (argc <2|| argc>3) {
        std::cerr << "Usage: " << argv[0] << " <xml platform file> (runReduced? true/false*)[--log=controller.threshold=info | --wrench-full-log]" << std::endl;
        exit(1);
    }
	if(argc==3){
		reduced=strcmp(argv[2],"true");
	}
    /* Instantiating the simulated platform */
    simulation->instantiatePlatform(argv[1]);

    /* Instantiate a storage service on the platform */
    //auto storage_service = simulation->add(new wrench::SimpleStorageService("StorageHost", {"/"}, {}, {}));

    /* Instantiate a bare-metal compute service on the platform */
    auto baremetal_service = simulation->add(new wrench::BareMetalComputeService("user", {"user"}, "", {}, {}));
	simulation->add(baremetal_service);

    /* Instantiate an execution controller */
    
	/*Construct XRootD tree that looks like
		    Root
	     /   |   \ 	
	Leaf1  Leaf2  Super1			
				 /   |   \ 	
			Leaf3  Leaf4  Super2
						 /   |   \ 	
					Leaf5  Leaf6  Super3
								 /   |   \ 	
							Leaf7  Leaf8  Super4
										 /   |   \ 	
									Leaf9  Leaf10  Leaf11
	        
	*/
	wrench::XRootD::XRootD xrootdManager(simulation,{{wrench::XRootD::Property::CACHE_MAX_LIFETIME,"28800"},{wrench::XRootD::Property::REDUCED_SIMULATION,to_string(reduced)}},{});
	std::shared_ptr<wrench::XRootD::Node> root=xrootdManager.createSupervisor("root");
	root->addChild(xrootdManager.createStorageServer("leaf1","/",{},{}));
	root->addChild(xrootdManager.createStorageServer("leaf2","/",{},{}));
	std::shared_ptr<wrench::XRootD::Node> activeNode=xrootdManager.createSupervisor("super1");
	root->addChild(activeNode);
	activeNode->addChild(xrootdManager.createStorageServer("leaf3","/",{},{}));
	activeNode->addChild(xrootdManager.createStorageServer("leaf4","/",{},{}));
	std::shared_ptr<wrench::XRootD::Node> previousNode=activeNode;
	activeNode=xrootdManager.createSupervisor("super2");
	previousNode->addChild(activeNode);
	activeNode->addChild(xrootdManager.createStorageServer("leaf5","/",{},{}));
	activeNode->addChild(xrootdManager.createStorageServer("leaf6","/",{},{}));
	previousNode=activeNode;
	activeNode=xrootdManager.createSupervisor("super3");
	previousNode->addChild(activeNode);
	activeNode->addChild(xrootdManager.createStorageServer("leaf7","/",{},{}));
	activeNode->addChild(xrootdManager.createStorageServer("leaf8","/",{},{}));
	previousNode=activeNode;
	activeNode=xrootdManager.createSupervisor("super4");
	previousNode->addChild(activeNode);
	activeNode->addChild(xrootdManager.createStorageServer("leaf9","/",{},{}));
	activeNode->addChild(xrootdManager.createStorageServer("leaf10","/",{},{}));
	activeNode->addChild(xrootdManager.createStorageServer("leaf11","/",{},{}));
	
	/* Launch the simulation */
	auto controller = simulation->add(        new wrench::Controller(baremetal_service, root,&xrootdManager, "root"));
    simulation->launch();

    return 0;
	
	}
