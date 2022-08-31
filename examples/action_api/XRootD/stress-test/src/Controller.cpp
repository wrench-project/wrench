
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
#include <wrench/services/storage/xrootd/Node.h>
#include "../include/Controller.h"
#include <iostream>
#include <chrono>
#include <ctime>    

std::string padLong(long l){
	
	if(l<10){
		return "0"+std::to_string(l);
	}else{
		return std::to_string(l);
	}
}
std::string padDouble(double l){
	
	if(l<10){
		return "0"+std::to_string(l);
	}else{
		return std::to_string(l);
	}
}
std::string formatDate(double time){
	if(time<0){
		return "Not Started";
	}
	long seconds=(long)time;
	double ms=time-seconds;
	long minutes=seconds/60;
	seconds%=60;
	long hours=minutes/60;
	minutes%=60;
	int days=hours/24;
	hours%=24;
	
	return std::to_string(days)+"-"+padLong(hours)+':'+padLong(minutes)+':'+padDouble(seconds+ms);
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
    Controller::Controller(std::shared_ptr<BareMetalComputeService> bare_metal_compute_service,
                           const std::shared_ptr<XRootD::Node> &root,
						   XRootD::XRootD *xrootdManager,
						   vector<std::shared_ptr<wrench::DataFile>> files,
                           const std::string &hostname) : ExecutionController(hostname, "controller"),
                                                          bare_metal_compute_service(bare_metal_compute_service), root(root),xrootdManager(xrootdManager),files(files) {}

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

        
        auto job_manager = this->createJobManager();

        WRENCH_INFO("Creating a compound job with an assortment of file reads");
        auto job1 = job_manager->createCompoundJob("job1");
		vector<std::shared_ptr<FileReadAction>> fileReads(files.size()*3);
		for(unsigned int i=0;i<files.size();i++){//add 3 actions for each read, first uncached, then cached, then uncached again
			fileReads[i]=job1->addFileReadAction("fileread"+to_string(i), files[i], root);
			fileReads[i+files.size()]=job1->addFileReadAction("fileread"+to_string(i+files.size()), files[i], root);
			fileReads[i+files.size()*2]=job1->addFileReadAction("fileread"+to_string(i+files.size()*2), files[i], root);
		
		}
        auto compute = job1->addComputeAction("compute", 500 * GFLOP, 50 * MBYTE, 1, 3, wrench::ParallelModel::AMDAHL(0.8));//should invalidate cache
		for(unsigned int i=0;i<files.size()*3-1;i++){//each read depends on the previous
			job1->addActionDependency(fileReads[i], fileReads[i+1]);
		}
		for(unsigned int i=0;i<files.size();i++){//each cached read depends on its own first read
			job1->addActionDependency(fileReads[i], fileReads[i+files.size()]);
		}
        for(unsigned int i=0;i<files.size();i++){//the final compute depends on the second read0
			job1->addActionDependency(fileReads[i+files.size()], compute);
		}
		for(unsigned int i=0;i<files.size();i++){//the 3rd read must happen after the cache invalidation
			job1->addActionDependency(compute,fileReads[i+files.size()*2]);
		}
		
        job_manager->submitJob(job1, this->bare_metal_compute_service);
		cout<<"initilization finished"<<endl;
		 auto start = std::chrono::system_clock::now();

        this->waitForAndProcessNextEvent();
		// Some computation here
		auto end = std::chrono::system_clock::now();
        WRENCH_INFO("Execution complete!");

        /*
		for (unsigned int i=0;i<fileReads.size();i++) {
			auto const &a=fileReads[i];
			std::cout<<std::right<<std::setfill(' ')<<
			"Action "<<std::setw(10)<<a->getName()<<": "<<
			std::setw(17)<<formatDate(a->getStartDate())<<" - "<<
			std::setw(17)<<formatDate(a->getEndDate())<<
			", Durration: "<<
			std::setw(7)<<a->getEndDate()-a->getStartDate()<<
			std::endl;
            //printf("Action %s: %.2fs - %.2fs, durration:%.2fs\n", a->getName().c_str(), a->getStartDate(), a->getEndDate(),a->getEndDate()-a->getStartDate());
        }
         */
		auto const &a=compute;
		std::cout<<std::right<<std::setfill(' ')<<
		"Action "<<std::setw(10)<<a->getName()<<": "<<
		std::setw(17)<<formatDate(a->getStartDate())<<" - "<<
		std::setw(17)<<formatDate(a->getEndDate())<<
		", Durration: "<<
		std::setw(7)<<a->getEndDate()-a->getStartDate()<<
		std::endl;
		cout<<"Real time taken: "<<formatDate(( std::chrono::duration_cast< std::chrono::milliseconds>(end-start)).count()/1000.0) <<endl;
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
