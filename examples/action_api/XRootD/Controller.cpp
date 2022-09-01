
/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** An execution controller to execute a workflow
 **/

#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MBYTE (1000.0 * 1000.0)
#define GBYTE (1000.0 * 1000.0 * 1000.0)

#include <iostream>
#include <iomanip>
#include <utility>
#include <wrench/services/storage/xrootd/Node.h>
#include "Controller.h"

/*
 * Helper function for pretty-printed output
 */
std::string padLong(long l){
	return (l < 10 ? "0"+std::to_string(l) : std::to_string(l));
}

std::string padDouble(double l){
	return (l < 10 ? "0"+std::to_string(l) : std::to_string(l));
}

std::string formatDate(double time){
	if(time<0){
		return "Not Started";
	}
	long seconds=(long)time;
	double ms = time - (double)seconds;
	long minutes = seconds / 60;
	seconds %= 60;
	long hours=minutes/60;
	minutes%=60;
	long days=hours/24;
	hours%=24;
	
	return std::to_string(days)+"-"+padLong(hours)+':'+padLong(minutes)+':'+padDouble((double)seconds+ms);
}

WRENCH_LOG_CATEGORY(controller, "Log category for Controller");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param bare_metal_compute_service: a set of compute services available to run actions
     * @param storage_services: a set of storage services available to store files
     * @param hostname: the name of the host on which to start the WMS
     */
    Controller::Controller(const std::shared_ptr<BareMetalComputeService> &bare_metal_compute_service,
                           const std::shared_ptr<XRootD::Node> &root,
						   XRootD::XRootD *xrootdManager,
                           const std::string &hostname) : ExecutionController(hostname, "controller"),
                                                          bare_metal_compute_service(bare_metal_compute_service), root(root),xrootdManager(xrootdManager) {}

    /**
     * @brief main method of the Controller
     *
     * @return 0 on completion
     *
     * @throw std::runtime_error
     */
    int Controller::main() {

        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);
        WRENCH_INFO("Controller starting");

        /* Add a bunch of 1-byte files to the simulation, which will
         * then be stored at storage servers in the XRootD tree
         */
        std::vector<std::shared_ptr<DataFile>> files ={ 
			wrench::Simulation::addFile("file01", 1 ),
			wrench::Simulation::addFile("file02", 1 ),
			wrench::Simulation::addFile("file03", 1 ),
			wrench::Simulation::addFile("file04", 1 ),
			wrench::Simulation::addFile("file05", 1 ),
			wrench::Simulation::addFile("file06", 1 ),
			wrench::Simulation::addFile("file07", 1),
			wrench::Simulation::addFile("file08", 1 ),
			wrench::Simulation::addFile("file09", 1 ),
			wrench::Simulation::addFile("file10", 1 ),
			wrench::Simulation::addFile("file11", 1),
			wrench::Simulation::addFile("file12", 1 ),
			wrench::Simulation::addFile("file13", 1 ),
			wrench::Simulation::addFile("file14", 1 ),
		};
        
			
        xrootdManager->createFile(files[0], root->getChild(0));//leaf 1
		xrootdManager->createFile(files[1], root->getChild(1));//leaf 2
		xrootdManager->createFile(files[2], root->getChild(2)->getChild(0));//leaf 3
		xrootdManager->createFile(files[3], root->getChild(2)->getChild(1));//leaf 4
		root->getChild(2)->getChild(2)->getChild(0)->createFile(files[4]);//leaf 5
		root->getChild(2)->getChild(2)->getChild(1)->createFile(files[5]);//leaf 6
		root->getChild(2)->getChild(2)->getChild(2)->getChild(0)->createFile(files[6]);//leaf 7
		root->getChild(2)->getChild(2)->getChild(2)->getChild(1)->createFile(files[7]);//leaf 8
		root->getChild(2)->getChild(2)->getChild(2)->getChild(2)->getChild(0)->createFile(files[8]);//leaf 9
		root->getChild(2)->getChild(2)->getChild(2)->getChild(2)->getChild(1)->createFile(files[9]);//leaf 10
		root->getChild(2)->getChild(2)->getChild(2)->getChild(2)->getChild(2)->createFile(files[10]);//leaf 11
		
		root->getChild(2)->getChild(2)->getChild(2)->getChild(2)->getChild(2)->createFile(files[12]);//File for supervisor tests
		
		root->getChild(2)->getChild(2)->getChild(2)->getChild(2)->getChild(2)->createFile(files[13]);//File for direct tests
	//  root  super1       super2       super3       super4       
        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        WRENCH_INFO("Creating a compound job with an assortment of file reads");
        auto job1 = job_manager->createCompoundJob("job1");
        auto fileread1 = job1->addFileReadAction("fileread1", files[0], root);//should be fast
		auto fileread2 = job1->addFileReadAction("fileread2", files[1], root);//should be equally fast
		auto fileread3 = job1->addFileReadAction("fileread3", files[2], root);//depth 1 search
		auto fileread4 = job1->addFileReadAction("fileread4", files[3], root);//depth 1 search
		auto fileread5 = job1->addFileReadAction("fileread5", files[4], root);//depth 2 search
		auto fileread6 = job1->addFileReadAction("fileread6", files[5], root);//depth 2 search
		auto fileread7 = job1->addFileReadAction("fileread7", files[6], root);//depth 3 search
		auto fileread8 = job1->addFileReadAction("fileread8", files[7], root);//depth 3 search
		auto fileread9 = job1->addFileReadAction("fileread9", files[8], root);//depth 4 search
		auto fileread10 = job1->addFileReadAction("fileread10", files[9], root);//depth 4 search
		auto fileread11 = job1->addFileReadAction("fileread11", files[10], root);//depth 4 search
		auto fileread12 = job1->addFileReadAction("fileread12", files[11], root);//this file does not exist
		auto fileread13 = job1->addFileReadAction("fileread13", files[10], root);//depth 4 search, but cached
		
		
		
		auto fileread14 = job1->addFileReadAction("fileread14", files[11], root);//this file does not exist
		
		auto fileread15 = job1->addFileReadAction("fileread15", files[10], root);//depth 4 search, but no longer cached
		auto fileread16 = job1->addFileReadAction("fileread16", files[12], root->getChild(2)->getChild(2)->getChild(2)->getChild(2));//check superviosor
		auto fileread17 = job1->addFileReadAction("fileread17", files[12], root->getChild(2)->getChild(2)->getChild(2)->getChild(2));//check superviosor cached
		auto fileread18 = job1->addFileReadAction("fileread18", files[13], root->getChild(2)->getChild(2)->getChild(2)->getChild(2)->getChild(2));//direct
		auto fileread19 = job1->addFileReadAction("fileread19", files[13], root->getChild(2)->getChild(2)->getChild(2)->getChild(2)->getChild(2));//direct cached
        auto compute = job1->addComputeAction("compute", 500 * GFLOP, 50 * MBYTE, 1, 3, wrench::ParallelModel::AMDAHL(0.8));//should invalidate cache
        job1->addActionDependency(fileread1, fileread2);
        job1->addActionDependency(fileread2, fileread3);
        job1->addActionDependency(fileread3, fileread4);
        job1->addActionDependency(fileread4, fileread5);
        job1->addActionDependency(fileread5, fileread6);
        job1->addActionDependency(fileread6, fileread7);
        job1->addActionDependency(fileread7, fileread8);
        job1->addActionDependency(fileread8, fileread9);
        job1->addActionDependency(fileread9, fileread10);
        job1->addActionDependency(fileread10, fileread11);
        job1->addActionDependency(fileread11, fileread13);
		job1->addActionDependency(fileread13,compute);
        job1->addActionDependency(compute, fileread12);
		
        job1->addActionDependency(compute, fileread15);
		
        job1->addActionDependency(fileread12, fileread14);//this task should never start
        
		
        job1->addActionDependency(fileread15, fileread16);
        job1->addActionDependency(fileread16, fileread17);
        job1->addActionDependency(fileread17, fileread18);
		
        job1->addActionDependency(fileread18, fileread19);
        job_manager->submitJob(job1, this->bare_metal_compute_service);
        this->waitForAndProcessNextEvent();

        WRENCH_INFO("Execution complete!");

        std::vector<std::shared_ptr<wrench::Action>> actions = {fileread1, fileread2, fileread3, fileread4,fileread5,fileread6,fileread7,fileread8,fileread9,fileread10,fileread11,fileread13,compute,fileread12,fileread15,fileread16,fileread17,fileread18,fileread19,fileread14};
		std::vector<std::string> comments={"should be fast","should be fast","depth 1 search","depth 1 search","depth 2 search","depth 2 search","depth 3 search","depth 3 search","depth 4 search","depth 4 search","depth 4 search","Depth 4, BUT cached","This long compute should invalidate the caches","This file Does not exist","depth 4, file should no longer be cached","depth 4, but directly from supervisor, should be depth 1","repeat but cached","direct leaf access","direct leaf access but cached","This action should not run"};
        for (unsigned int i=0;i<actions.size();i++) {
			auto const &a=actions[i];
			std::cout<<std::right<<std::setfill(' ') 
			<<"Action "<<std::setw(10)<<a->getName()<<": "
			<<std::setw(17)<<formatDate(a->getStartDate())
			<<" - "
			<<std::setw(17)<<formatDate(a->getEndDate())
			<<", Durration: "
			<<std::setw(7)<<a->getEndDate()-a->getStartDate()
			<<" Comment: "
			<< comments[i]<<std::endl;
            //printf("Action %s: %.2fs - %.2fs, durration:%.2fs\n", a->getName().c_str(), a->getStartDate(), a->getEndDate(),a->getEndDate()-a->getStartDate());
        }
        return 0;
    }

    /**
     * @brief Process a compound job completion event
     *
     * @param event: the event
     */
    void Controller::processEventCompoundJobCompletion(std::shared_ptr<CompoundJobCompletedEvent> event) {
        /* Retrieve the job that this event is for */
        auto job = event->job;
        /* Print info about all actions in the job */
        WRENCH_INFO("Notified that compound job %s has completed:", job->getName().c_str());
    }
}// namespace wrench
