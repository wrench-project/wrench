
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
#include "../include/platformCreator.h"
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
 int LEAFS=100;
 int FILES=100;
 const double DENSITY=.75;
 const double REDUNDANCY=.25;
 const double FILE_SIZE=1;
int main(int argc, char **argv) {

    /* Create a WRENCH simulation object */
    auto simulation = wrench::Simulation::createSimulation();

    /* Initialize the simulation */
    simulation->init(&argc, argv);
	
	std::string reduced="false";
    /* Parsing of the command-line arguments */
    if (argc<1||argc>4) {
        std::cerr << "Usage: " << argv[0] << " [size] (runReduced? true/false*)[--log=controller.threshold=info | --wrench-full-log]" << std::endl;
        exit(1);
    }
	LEAFS=atoi(argv[1]);
	FILES=LEAFS;
	if(argc==3){
		reduced=argv[2];
	}
	wrench::XRootD::XRootD xrootdManager(simulation,{{wrench::XRootD::Property::CACHE_MAX_LIFETIME,"28800"},{wrench::XRootD::Property::REDUCED_SIMULATION,reduced}},{});
    /* Instantiating the simulated platform */
	PlatformCreator platform(xrootdManager,DENSITY,LEAFS);
	platform.ret=make_shared<Return>();
    simulation->instantiatePlatform(platform);

    /* Instantiate a storage service on the platform */
    //auto storage_service = simulation->add(new wrench::SimpleStorageService("StorageHost", {"/"}, {}, {}));

    /* Instantiate a bare-metal compute service on the platform */
    auto baremetal_service = simulation->add(new wrench::BareMetalComputeService("user", {"user"}, "", {}, {}));
	simulation->add(baremetal_service);


	vector<std::shared_ptr<wrench::DataFile>> files;
	for(int i=0;i<FILES;i++){
		auto file=wrench::Simulation::addFile("file"+to_string(i),FILE_SIZE);
		files.push_back(file);
		double copies=((std::rand()/(double)(RAND_MAX + 1u))*REDUNDANCY);
		//cout<<copies<<endl;
		while(copies>=0){
			int index=std::rand()/((RAND_MAX + 1u)/platform.ret->fileServers.size());
			platform.ret->fileServers[index]->createFile(file);
			copies--;
		}
	}
    
	
	/* Launch the simulation */
	auto controller = simulation->add(        new wrench::Controller(baremetal_service, platform.ret->root,&xrootdManager,files, "root"));
	
    simulation->launch();

    return 0;
	
	}
