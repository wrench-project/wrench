/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <gtest/gtest.h>

#include <wrench-dev.h>
#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MBYTE (1000.0 * 1000.0)
#define GBYTE (1000.0 * 1000.0 * 1000.0)

WRENCH_LOG_CATEGORY(xrootd_storage_service_full_functional_test, "Log category for XRootDServiceFullFunctionalTest");


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
    if(time < 0){
        return "Not Started";
    }
    long seconds = (long)time;
    double ms = time - (double)seconds;
    long minutes = seconds / 60;
    seconds %= 60;
    long hours = minutes / 60;
    minutes %= 60;
    long days = hours / 24;
    hours %= 24;

    return std::to_string(days)+"-"+padLong(hours)+':'+padLong(minutes)+':'+padDouble((double)seconds+ms);
}

class XRootDServiceFunctionalTest : public ::testing::Test {

public:

    void do_FullFunctionality_test();

    std::shared_ptr<wrench::XRootD::Node> root_supervisor;

protected:

    XRootDServiceFunctionalTest() {
    }

};


/**********************************************************************/
/**  FULL FUNCTIONALITY SIMULATION TEST                             **/
/**********************************************************************/

class XRootDServiceFullFunctionalityTestExecutionController : public wrench::ExecutionController {

public:
    XRootDServiceFullFunctionalityTestExecutionController(XRootDServiceFunctionalTest *test,
                                                          wrench::XRootD::XRootDDeployment *xrootd_deployment,
                                                          std::shared_ptr<wrench::BareMetalComputeService> bare_metal_compute_service,
                                                          std::string hostname) :
            wrench::ExecutionController(hostname, "test"), test(test),
            xrootd_deployment(xrootd_deployment),
            bare_metal_compute_service(bare_metal_compute_service) {}

private:
    XRootDServiceFunctionalTest *test;
    wrench::XRootD::XRootDDeployment *xrootd_deployment;
    std::shared_ptr<wrench::BareMetalComputeService> bare_metal_compute_service;

    int main() override {
        /* Set the logging output to GREEN */
        wrench::TerminalOutput::setThisProcessLoggingColor(wrench::TerminalOutput::COLOR_GREEN);
        WRENCH_INFO("Controller starting");

        /* Add a bunch of 1-byte files to the simulation, which will
         * then be stored at storage servers in the XRootD tree
         */
        std::vector<std::shared_ptr<wrench::DataFile>> files ={
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

        auto root = this->xrootd_deployment->getRootSupervisor();


        root->getChild(0)->createFile(files[0]);
        root->getChild(1)->createFile(files[1]);
        root->getChild(2)->getChild(0)->createFile(files[2]);
        root->getChild(2)->getChild(1)->createFile(files[3]);
        root->getChild(2)->getChild(2)->getChild(0)->createFile(files[4]);//leaf 5
        root->getChild(2)->getChild(2)->getChild(1)->createFile(files[5]);//leaf 6
        root->getChild(2)->getChild(2)->getChild(2)->getChild(0)->createFile(files[6]);//leaf 7
        root->getChild(2)->getChild(2)->getChild(2)->getChild(1)->createFile(files[7]);//leaf 8
        root->getChild(2)->getChild(2)->getChild(2)->getChild(2)->getChild(0)->createFile(files[8]);//leaf 9
        root->getChild(2)->getChild(2)->getChild(2)->getChild(2)->getChild(1)->createFile(files[9]);//leaf 10
        root->getChild(2)->getChild(2)->getChild(2)->getChild(2)->getChild(2)->createFile(files[10]);//leaf 11

        root->getChild(2)->getChild(2)->getChild(2)->getChild(2)->getChild(2)->createFile(files[12]);//File for supervisor tests

        root->getChild(2)->getChild(2)->getChild(2)->getChild(2)->getChild(2)->createFile(files[13]);//File for direct tests

        try {
            root->createFile(files[0]);
            throw std::runtime_error("Should not be able to create a file on a non-storage node");
        } catch (std::runtime_error &ignore) {}

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

        // TODO: Add comment that compute is super long to invalidate cachue

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
            WRENCH_INFO("Action %s: %.2fs - %.2fs, duration: %.2fs | %s",
                        a->getName().c_str(), a->getStartDate(), a->getEndDate(),
                        a->getEndDate() - a->getStartDate(), comments[i].c_str());
        }
        return 0;
    }

    /**
     * @brief Process a compound job completion eventj
     *
     * @param event: the event
     */
    void processEventCompoundJobCompletion(std::shared_ptr<wrench::CompoundJobCompletedEvent> event) override {
        /* Retrieve the job that this event is for */
        auto job = event->job;
        /* Print info about all actions in the job */
        WRENCH_INFO("Notified that compound job %s has completed:", job->getName().c_str());
    }
};

TEST_F(XRootDServiceFunctionalTest, FullFunctionality) {
    DO_TEST_WITH_FORK(do_FullFunctionality_test);
}

void XRootDServiceFunctionalTest::do_FullFunctionality_test() {

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
//    argv[1] = strdup("--wrench-full-log");

/* Create a WRENCH simulation object */
    auto simulation = wrench::Simulation::createSimulation();

/* Initialize the simulation */
    simulation->init(&argc, argv);

/* Instantiating the simulated platform */
    std::string platform_file_path = "test/platform_files/xrootd_platform.xml";
    try {
        simulation->instantiatePlatform(platform_file_path);
    } catch (std::exception &e) {
        platform_file_path = "../test/platform_files/xrootd_platform.xml";
        simulation->instantiatePlatform(platform_file_path);
    }

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
    auto controller = simulation->add(new XRootDServiceFullFunctionalityTestExecutionController(this, &xrootd_deployment, baremetal_service, "root"));

/* Launch the simulation */
    simulation->launch();

}