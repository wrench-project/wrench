/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench-dev.h>
#include <simgrid_S4U_util/S4U_Mailbox.h>
#include <simulation/SimulationMessage.h>
#include "services/compute/standard_job_executor/StandardJobExecutorMessage.h"
#include <gtest/gtest.h>
#include <wrench/services/batch_service/BatchService.h>
#include "NoopScheduler.h"

#include "TestWithFork.h"


class BatchServiceTest : public ::testing::Test {

public:
    wrench::StorageService *storage_service1 = nullptr;
    wrench::StorageService *storage_service2 = nullptr;
    wrench::ComputeService *compute_service = nullptr;
    wrench::Simulation *simulation;


    void do_StandardJobTaskTest_test();
    void do_PilotJobTaskTest_test();
    void do_StandardPlusPilotJobTaskTest_test();
    void do_InsufficientCoresTaskTest_test();
    void do_BestFitTaskTest_test();
    void do_noArgumentsJobSubmissionTest();


protected:
    BatchServiceTest() {

        // Create the simplest workflow
        workflow = new wrench::Workflow();

        // Create a four-host 10-core platform file
        std::string xml = "<?xml version='1.0'?>"
                "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                "<platform version=\"4\"> "
                "   <AS id=\"AS0\" routing=\"Full\"> "
                "       <host id=\"Host1\" speed=\"1f\" core=\"10\"/> "
                "       <host id=\"Host2\" speed=\"1f\" core=\"10\"/> "
                "       <host id=\"Host3\" speed=\"1f\" core=\"10\"/> "
                "       <host id=\"Host4\" speed=\"1f\" core=\"10\"/> "
                "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\""
                "/> </route>"
                "   </AS> "
                "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

    }

    std::string platform_file_path = "/tmp/platform.xml";
    wrench::Workflow *workflow;

};

/**********************************************************************/
/**  ONE STANDARD JOB SUBMISSION TASK SIMULATION TEST ON ONE HOST                **/
/**********************************************************************/

class OneStandardJobSubmissionTestWMS : public wrench::WMS {

public:
    OneStandardJobSubmissionTestWMS(BatchServiceTest *test,
                             wrench::Workflow *workflow,
                             std::unique_ptr<wrench::Scheduler> scheduler,
                             std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
        this->test = test;
    }


private:

    BatchServiceTest *test;

    int main() {
        // Create a job manager
        std::unique_ptr<wrench::JobManager> job_manager =
                std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));
        {
            // Create a sequential task that lasts one min and requires 2 cores
            wrench::WorkflowTask *task = this->workflow->addTask("task", 60, 2, 2, 1.0);
            task->addInputFile(workflow->getFileById("input_file"));
            task->addOutputFile(workflow->getFileById("output_file"));


            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

            wrench::StandardJob *job = job_manager->createStandardJob(
                    {task},
                    {
                            {*(task->getInputFiles().begin()),  this->test->storage_service1},
                            {*(task->getOutputFiles().begin()), this->test->storage_service1}
                    },
                    {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                            workflow->getFileById("input_file"), this->test->storage_service1,
                            this->test->storage_service2)},
                    {},
                    {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileById("input_file"),
                                                                                  this->test->storage_service2)});

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "1";
            batch_job_args["-t"] = "5"; //time in minutes
            batch_job_args["-c"] = "4"; //number of cores per node
            try {
                job_manager->submitJob(job, this->test->compute_service, batch_job_args);
            }catch (wrench::WorkflowExecutionException &e){
                throw std::runtime_error(
                        "Got some exception"
                );
            }

            // Wait for a workflow execution event
            std::unique_ptr<wrench::WorkflowExecutionEvent> event;
            try {
                event = workflow->waitForNextExecutionEvent();
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            switch (event->type) {
                case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
                    // success, do nothing for now
                    break;
                }
                default: {
                    throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
                }
            }
            workflow->removeTask(task);
        }



        // Terminate everything
        this->simulation->shutdownAllComputeServices();
        this->simulation->shutdownAllStorageServices();
        this->simulation->getFileRegistryService()->stop();
        return 0;
    }
};

TEST_F(BatchServiceTest, OneStandardJobSubmissionTest) {
    DO_TEST_WITH_FORK(do_StandardJobTaskTest_test);
}


void BatchServiceTest::do_StandardJobTaskTest_test() {

    // Create and initialize a simulation
    wrench::Simulation *simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("batch_service_test");

    EXPECT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = simulation->getHostnameList()[0];

    // Create a WMS
    EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
            std::unique_ptr<wrench::WMS>(new OneStandardJobSubmissionTestWMS(this, workflow,
                                                                                              std::unique_ptr<wrench::Scheduler>(
                            new NoopScheduler()), hostname))));

    // Create a Storage Service
    EXPECT_NO_THROW(storage_service1 = simulation->add(
            std::unique_ptr<wrench::SimpleStorageService>(
                    new wrench::SimpleStorageService(hostname, 10000000000000.0))));

    // Create a Storage Service
    EXPECT_NO_THROW(storage_service2 = simulation->add(
            std::unique_ptr<wrench::SimpleStorageService>(
                    new wrench::SimpleStorageService(hostname, 10000000000000.0))));

    // Create a Batch Service
    EXPECT_NO_THROW(compute_service = simulation->add(
            std::unique_ptr<wrench::BatchService>(
                    new wrench::BatchService(hostname,simulation->getHostnameList(),
                                             storage_service1,true,true,{}))));

    std::unique_ptr<wrench::FileRegistryService> file_registry_service(
            new wrench::FileRegistryService(hostname));

    simulation->setFileRegistryService(std::move(file_registry_service));

    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
    wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

    // Staging the input_file on the storage service
    EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service1));


    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    EXPECT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}

/**********************************************************************/
/**  ONE PILOT JOB SUBMISSION TASK SIMULATION TEST ON ONE HOST                **/
/**********************************************************************/

class OnePilotJobSubmissionTestWMS : public wrench::WMS {

public:
    OnePilotJobSubmissionTestWMS(BatchServiceTest *test,
                                 wrench::Workflow *workflow,
                                 std::unique_ptr<wrench::Scheduler> scheduler,
                                 std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
        this->test = test;
    }


private:

    BatchServiceTest *test;

    int main() {
        // Create a job manager
        std::unique_ptr<wrench::JobManager> job_manager =
                std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));
        {
            // Create a pilot job
            wrench::PilotJob *pilot_job = job_manager->createPilotJob(this->workflow, 1, 30);

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "1";
            batch_job_args["-t"] = "1"; //time in minutes
            batch_job_args["-c"] = "4"; //number of cores per node

            // Submit a pilot job
            try {
                job_manager->submitJob((wrench::WorkflowJob*)pilot_job, this->test->compute_service, batch_job_args);
            } catch (wrench::WorkflowExecutionException &e){
                throw std::runtime_error(
                        "Got some exception "+std::string(e.what())
                );
            }

            // Wait for a workflow execution event
            std::unique_ptr<wrench::WorkflowExecutionEvent> event;
            try {
                event = workflow->waitForNextExecutionEvent();
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            switch (event->type) {
                case wrench::WorkflowExecutionEvent::PILOT_JOB_START: {
                    // success, do nothing for now
                    break;
                }
                default: {
                    throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
                }
            }

            try {
                event = workflow->waitForNextExecutionEvent();
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            switch (event->type) {
                case wrench::WorkflowExecutionEvent::PILOT_JOB_EXPIRATION: {
                    // success, do nothing for now
                    break;
                }
                default: {
                    throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
                }
            }
        }

        // Terminate everything
        this->simulation->shutdownAllComputeServices();
        this->simulation->shutdownAllStorageServices();
        this->simulation->getFileRegistryService()->stop();
        return 0;
    }
};

TEST_F(BatchServiceTest, OnePilotJobSubmissionTest) {
    DO_TEST_WITH_FORK(do_PilotJobTaskTest_test);
}

void BatchServiceTest::do_PilotJobTaskTest_test() {

    // Create and initialize a simulation
    wrench::Simulation *simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("batch_service_test");

    EXPECT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = simulation->getHostnameList()[0];

    // Create a WMS
    EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
            std::unique_ptr<wrench::WMS>(new OnePilotJobSubmissionTestWMS(this, workflow,
                                                                          std::unique_ptr<wrench::Scheduler>(
                            new NoopScheduler()), hostname))));

    // Create a Storage Service
    EXPECT_NO_THROW(storage_service1 = simulation->add(
            std::unique_ptr<wrench::SimpleStorageService>(
                    new wrench::SimpleStorageService(hostname, 10000000000000.0))));

    // Create a Storage Service
    EXPECT_NO_THROW(storage_service2 = simulation->add(
            std::unique_ptr<wrench::SimpleStorageService>(
                    new wrench::SimpleStorageService(hostname, 10000000000000.0))));

    // Create a Batch Service
    EXPECT_NO_THROW(compute_service = simulation->add(
            std::unique_ptr<wrench::BatchService>(
                    new wrench::BatchService(hostname,simulation->getHostnameList(),
                                             storage_service1,true,true,{}))));

    std::unique_ptr<wrench::FileRegistryService> file_registry_service(
            new wrench::FileRegistryService(hostname));

    simulation->setFileRegistryService(std::move(file_registry_service));

    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
    wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

    // Staging the input_file on the storage service
    EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service1));


    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    EXPECT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}


/**********************************************************************/
/**  STANDARD + PILOT JOB SUBMISSION TASK SIMULATION TEST ON ONE-ONE HOST                **/
/**********************************************************************/

class StandardPlusPilotJobSubmissionTestWMS : public wrench::WMS {

public:
    StandardPlusPilotJobSubmissionTestWMS(BatchServiceTest *test,
                                          wrench::Workflow *workflow,
                                          std::unique_ptr<wrench::Scheduler> scheduler,
                                          std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
        this->test = test;
    }


private:

    BatchServiceTest *test;

    int main() {
        // Create a job manager
        std::unique_ptr<wrench::JobManager> job_manager =
                std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));
        {
            // Create a sequential task that lasts one min and requires 2 cores
            wrench::WorkflowTask *task = this->workflow->addTask("task", 50, 2, 2, 1.0);
            task->addInputFile(workflow->getFileById("input_file"));
            task->addOutputFile(workflow->getFileById("output_file"));

            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

            wrench::StandardJob *job = job_manager->createStandardJob(
                    {task},
                    {
                            {*(task->getInputFiles().begin()),  this->test->storage_service1},
                            {*(task->getOutputFiles().begin()), this->test->storage_service1}
                    },
                    {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                            workflow->getFileById("input_file"), this->test->storage_service1,
                            this->test->storage_service2)},
                    {},
                    {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileById("input_file"),
                                                                                  this->test->storage_service2)});

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "1";
            batch_job_args["-t"] = "2"; //time in minutes
            batch_job_args["-c"] = "4"; //number of cores per node
            try {
                job_manager->submitJob(job, this->test->compute_service, batch_job_args);
            }catch (wrench::WorkflowExecutionException &e){
                throw std::runtime_error(
                        "Got some exception"
                );
            }
            // Wait for a workflow execution event
            std::unique_ptr<wrench::WorkflowExecutionEvent> event;
            try {
                event = workflow->waitForNextExecutionEvent();
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            switch (event->type) {
                case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
                    // success, do nothing for now
                    break;
                }
                default: {
                    throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
                }
            }
            workflow->removeTask(task);
        }

        {
            // Create a pilot job
            wrench::PilotJob *pilot_job = job_manager->createPilotJob(this->workflow, 1, 30);

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "1";
            batch_job_args["-t"] = "1"; //time in minutes
            batch_job_args["-c"] = "4"; //number of cores per node

            // Submit a pilot job
            try {
                job_manager->submitJob((wrench::WorkflowJob*)pilot_job, this->test->compute_service, batch_job_args);
            } catch (wrench::WorkflowExecutionException &e){
                throw std::runtime_error(
                        "Got some exception "+std::string(e.what())
                );
            }

            // Wait for a workflow execution event
            std::unique_ptr<wrench::WorkflowExecutionEvent> event;
            try {
                event = workflow->waitForNextExecutionEvent();
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            switch (event->type) {
                case wrench::WorkflowExecutionEvent::PILOT_JOB_START: {
                    // success, do nothing for now
                    break;
                }
                default: {
                    throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
                }
            }

            try {
                event = workflow->waitForNextExecutionEvent();
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            switch (event->type) {
                case wrench::WorkflowExecutionEvent::PILOT_JOB_EXPIRATION: {
                    // success, do nothing for now
                    break;
                }
                default: {
                    throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
                }
            }
        }


        // Terminate everything
        this->simulation->shutdownAllComputeServices();
        this->simulation->shutdownAllStorageServices();
        this->simulation->getFileRegistryService()->stop();
        return 0;
    }
};

TEST_F(BatchServiceTest, StandardPlusPilotJobSubmissionTest) {
    DO_TEST_WITH_FORK(do_StandardPlusPilotJobTaskTest_test);
}

void BatchServiceTest::do_StandardPlusPilotJobTaskTest_test() {

    // Create and initialize a simulation
    wrench::Simulation *simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("batch_service_test");

    EXPECT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = simulation->getHostnameList()[0];

    // Create a WMS
    EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
            std::unique_ptr<wrench::WMS>(new StandardPlusPilotJobSubmissionTestWMS(this, workflow,
                                                                                   std::unique_ptr<wrench::Scheduler>(
                            new NoopScheduler()), hostname))));

    // Create a Storage Service
    EXPECT_NO_THROW(storage_service1 = simulation->add(
            std::unique_ptr<wrench::SimpleStorageService>(
                    new wrench::SimpleStorageService(hostname, 10000000000000.0))));

    // Create a Storage Service
    EXPECT_NO_THROW(storage_service2 = simulation->add(
            std::unique_ptr<wrench::SimpleStorageService>(
                    new wrench::SimpleStorageService(hostname, 10000000000000.0))));

    // Create a Batch Service
    EXPECT_NO_THROW(compute_service = simulation->add(
            std::unique_ptr<wrench::BatchService>(
                    new wrench::BatchService(hostname,simulation->getHostnameList(),
                                             storage_service1,true,true,{}))));

    std::unique_ptr<wrench::FileRegistryService> file_registry_service(
            new wrench::FileRegistryService(hostname));

    simulation->setFileRegistryService(std::move(file_registry_service));

    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
    wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

    // Staging the input_file on the storage service
    EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service1));


    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    EXPECT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}


/**********************************************************************/
/**  INSUFFICIENT CORES JOB SUBMISSION TASK SIMULATION TEST ON ONE-ONE HOST                **/
/**********************************************************************/

class InsufficientCoresJobSubmissionTestWMS : public wrench::WMS {

public:
    InsufficientCoresJobSubmissionTestWMS(BatchServiceTest *test,
                                          wrench::Workflow *workflow,
                                          std::unique_ptr<wrench::Scheduler> scheduler,
                                          std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
        this->test = test;
    }


private:

    BatchServiceTest *test;

    int main() {
        // Create a job manager
        std::unique_ptr<wrench::JobManager> job_manager =
                std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));
        {
            // Create a sequential task that lasts one min and requires 2 cores
            wrench::WorkflowTask *task = this->workflow->addTask("task", 50, 2, 12, 1.0);
            task->addInputFile(workflow->getFileById("input_file"));
            task->addOutputFile(workflow->getFileById("output_file"));

            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

            wrench::StandardJob *job = job_manager->createStandardJob(
                    {task},
                    {
                            {*(task->getInputFiles().begin()),  this->test->storage_service1},
                            {*(task->getOutputFiles().begin()), this->test->storage_service1}
                    },
                    {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                            workflow->getFileById("input_file"), this->test->storage_service1,
                            this->test->storage_service2)},
                    {},
                    {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileById("input_file"),
                                                                                  this->test->storage_service2)});

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "1";
            batch_job_args["-t"] = "2"; //time in minutes
            batch_job_args["-c"] = "12"; //number of cores per node
            bool success = false;
            try {
                job_manager->submitJob(job, this->test->compute_service, batch_job_args);
            }catch (std::runtime_error e){
                success = true;
            }
            if (not success){
                throw std::runtime_error(
                        "Expecting a runtime error of not enough arugments but did not get any"
                );
            }

            workflow->removeTask(task);
        }


        // Terminate everything
        this->simulation->shutdownAllComputeServices();
        this->simulation->shutdownAllStorageServices();
        this->simulation->getFileRegistryService()->stop();
        return 0;
    }
};

TEST_F(BatchServiceTest, InsufficientCoresJobSubmissionTest) {
    DO_TEST_WITH_FORK(do_InsufficientCoresTaskTest_test);
}

void BatchServiceTest::do_InsufficientCoresTaskTest_test() {

    // Create and initialize a simulation
    wrench::Simulation *simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("batch_service_test");

    EXPECT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = simulation->getHostnameList()[0];

    // Create a WMS
    EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
            std::unique_ptr<wrench::WMS>(new InsufficientCoresJobSubmissionTestWMS(this, workflow,
                                                                                   std::unique_ptr<wrench::Scheduler>(
                            new NoopScheduler()), hostname))));

    // Create a Storage Service
    EXPECT_NO_THROW(storage_service1 = simulation->add(
            std::unique_ptr<wrench::SimpleStorageService>(
                    new wrench::SimpleStorageService(hostname, 10000000000000.0))));

    // Create a Storage Service
    EXPECT_NO_THROW(storage_service2 = simulation->add(
            std::unique_ptr<wrench::SimpleStorageService>(
                    new wrench::SimpleStorageService(hostname, 10000000000000.0))));

    // Create a Batch Service
    EXPECT_NO_THROW(compute_service = simulation->add(
            std::unique_ptr<wrench::BatchService>(
                    new wrench::BatchService(hostname,simulation->getHostnameList(),
                                             storage_service1,true,true,{}))));

    std::unique_ptr<wrench::FileRegistryService> file_registry_service(
            new wrench::FileRegistryService(hostname));

    simulation->setFileRegistryService(std::move(file_registry_service));

    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
    wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

    // Staging the input_file on the storage service
    EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service1));


    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    EXPECT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}



/**********************************************************************/
/**  NO ARGUMENTS JOB SUBMISSION TASK SIMULATION TEST ON ONE-ONE HOST                **/
/**********************************************************************/

class NoArgumentsJobSubmissionTestWMS : public wrench::WMS {

public:
    NoArgumentsJobSubmissionTestWMS(BatchServiceTest *test,
                                          wrench::Workflow *workflow,
                                          std::unique_ptr<wrench::Scheduler> scheduler,
                                          std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
        this->test = test;
    }


private:

    BatchServiceTest *test;

    int main() {
        // Create a job manager
        std::unique_ptr<wrench::JobManager> job_manager =
                std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));
        {
            // Create a sequential task that lasts one min and requires 2 cores
            wrench::WorkflowTask *task = this->workflow->addTask("task", 50, 2, 2, 1.0);
            task->addInputFile(workflow->getFileById("input_file"));
            task->addOutputFile(workflow->getFileById("output_file"));

            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

            wrench::StandardJob *job = job_manager->createStandardJob(
                    {task},
                    {
                            {*(task->getInputFiles().begin()),  this->test->storage_service1},
                            {*(task->getOutputFiles().begin()), this->test->storage_service1}
                    },
                    {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                            workflow->getFileById("input_file"), this->test->storage_service1,
                            this->test->storage_service2)},
                    {},
                    {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileById("input_file"),
                                                                                  this->test->storage_service2)});

            std::map<std::string, std::string> batch_job_args;
            bool success = false;
            try {
                job_manager->submitJob(job, this->test->compute_service, batch_job_args);
            }catch (std::invalid_argument e){
                success = true;
            }
            if (not success){
                throw std::runtime_error(
                        "Expecting a runtime error of not arguments but did not get any such exceptions"
                );
            }

            workflow->removeTask(task);
        }


        // Terminate everything
        this->simulation->shutdownAllComputeServices();
        this->simulation->shutdownAllStorageServices();
        this->simulation->getFileRegistryService()->stop();
        return 0;
    }
};

TEST_F(BatchServiceTest, NoArgumentsJobSubmissionTes) {
    DO_TEST_WITH_FORK(do_noArgumentsJobSubmissionTest);
}

void BatchServiceTest::do_noArgumentsJobSubmissionTest() {

    // Create and initialize a simulation
    wrench::Simulation *simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("batch_service_test");

    EXPECT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = simulation->getHostnameList()[0];

    // Create a WMS
    EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
            std::unique_ptr<wrench::WMS>(new NoArgumentsJobSubmissionTestWMS(this, workflow,
                                                                                   std::unique_ptr<wrench::Scheduler>(
                            new NoopScheduler()), hostname))));

    // Create a Storage Service
    EXPECT_NO_THROW(storage_service1 = simulation->add(
            std::unique_ptr<wrench::SimpleStorageService>(
                    new wrench::SimpleStorageService(hostname, 10000000000000.0))));

    // Create a Storage Service
    EXPECT_NO_THROW(storage_service2 = simulation->add(
            std::unique_ptr<wrench::SimpleStorageService>(
                    new wrench::SimpleStorageService(hostname, 10000000000000.0))));

    // Create a Batch Service
    EXPECT_NO_THROW(compute_service = simulation->add(
            std::unique_ptr<wrench::BatchService>(
                    new wrench::BatchService(hostname,simulation->getHostnameList(),
                                             storage_service1,true,true,{}))));

    std::unique_ptr<wrench::FileRegistryService> file_registry_service(
            new wrench::FileRegistryService(hostname));

    simulation->setFileRegistryService(std::move(file_registry_service));

    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
    wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

    // Staging the input_file on the storage service
    EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service1));


    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    EXPECT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}


/**********************************************************************/
/**  BEST FIT STANDARD JOB SUBMISSION TASK SIMULATION TEST ON ONE-ONE HOST                **/
/**********************************************************************/

class BestFitStandardJobSubmissionTestWMS : public wrench::WMS {

public:
    BestFitStandardJobSubmissionTestWMS(BatchServiceTest *test,
                                          wrench::Workflow *workflow,
                                          std::unique_ptr<wrench::Scheduler> scheduler,
                                          std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
        this->test = test;
    }


private:

    BatchServiceTest *test;

    int main() {
        // Create a job manager
        std::unique_ptr<wrench::JobManager> job_manager =
                std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));
        {
            // Create a sequential task that lasts one min and requires 8 cores
            wrench::WorkflowTask *task = this->workflow->addTask("task", 50, 8, 8, 1.0);
            task->addInputFile(workflow->getFileById("input_file"));
            task->addOutputFile(workflow->getFileById("output_file"));

            //Create another sequential task that lasts one min and requires 9 cores
            wrench::WorkflowTask *task1 = this->workflow->addTask("task1", 50, 9, 9, 1.0);
            task1->addInputFile(workflow->getFileById("input_file_1"));
            task1->addOutputFile(workflow->getFileById("output_file_1"));

            //Create another sequential task that lasts one min and requires 1 cores
            wrench::WorkflowTask *task2 = this->workflow->addTask("task2", 50, 1, 1, 1.0);
            task2->addInputFile(workflow->getFileById("input_file_2"));
            task2->addOutputFile(workflow->getFileById("output_file_2"));

            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)
            wrench::StandardJob *job = job_manager->createStandardJob(
                    {task},
                    {
                            {*(task->getInputFiles().begin()),  this->test->storage_service1},
                            {*(task->getOutputFiles().begin()), this->test->storage_service1}
                    },
                    {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                            workflow->getFileById("input_file"), this->test->storage_service1,
                            this->test->storage_service2)},
                    {},
                    {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileById("input_file"),
                                                                                  this->test->storage_service2)});

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "1";
            batch_job_args["-t"] = "2"; //time in minutes
            batch_job_args["-c"] = "8"; //number of cores per node
            try {
                job_manager->submitJob(job, this->test->compute_service, batch_job_args);
            }catch (wrench::WorkflowExecutionException &e){
                throw std::runtime_error(
                        "Got some exception"
                );
            }

            wrench::StandardJob *job1 = job_manager->createStandardJob(
                    {task1},
                    {
                            {*(task1->getInputFiles().begin()),  this->test->storage_service1},
                            {*(task1->getOutputFiles().begin()), this->test->storage_service1}
                    },
                    {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                            workflow->getFileById("input_file_1"), this->test->storage_service1,
                            this->test->storage_service2)},
                    {},
                    {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileById("input_file_1"),
                                                                                  this->test->storage_service2)});

            std::map<std::string, std::string> task1_batch_job_args;
            task1_batch_job_args["-N"] = "1";
            task1_batch_job_args["-t"] = "2"; //time in minutes
            task1_batch_job_args["-c"] = "9"; //number of cores per node
            try {
                job_manager->submitJob(job1, this->test->compute_service, task1_batch_job_args);
            }catch (wrench::WorkflowExecutionException &e){
                throw std::runtime_error(
                        "Got some exception"
                );
            }

            wrench::StandardJob *job2 = job_manager->createStandardJob(
                    {task2},
                    {
                            {*(task2->getInputFiles().begin()),  this->test->storage_service1},
                            {*(task2->getOutputFiles().begin()), this->test->storage_service1}
                    },
                    {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                            workflow->getFileById("input_file_2"), this->test->storage_service1,
                            this->test->storage_service2)},
                    {},
                    {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileById("input_file_2"),
                                                                                  this->test->storage_service2)});

            std::map<std::string, std::string> task2_batch_job_args;
            task2_batch_job_args["-N"] = "1";
            task2_batch_job_args["-t"] = "2"; //time in minutes
            task2_batch_job_args["-c"] = "1"; //number of cores per node
            try {
                job_manager->submitJob(job2, this->test->compute_service, task2_batch_job_args);
            }catch (wrench::WorkflowExecutionException &e){
                throw std::runtime_error(
                        "Got some exception"
                );
            }

            //wait for three standard job completion events
            int num_events = 0;
            while(num_events<3) {
                std::unique_ptr<wrench::WorkflowExecutionEvent> event;
                try {
                    event = workflow->waitForNextExecutionEvent();
                } catch (wrench::WorkflowExecutionException &e) {
                    throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
                }
                switch (event->type) {
                    case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
                        // success, do nothing for now
                        break;
                    }
                    default: {
                        throw std::runtime_error(
                                "Unexpected workflow execution event: " + std::to_string((int) (event->type)));
                    }
                }
                num_events++;
            }
            workflow->removeTask(task);
            workflow->removeTask(task1);
            workflow->removeTask(task2);
        }

        // Terminate everything
        this->simulation->shutdownAllComputeServices();
        this->simulation->shutdownAllStorageServices();
        this->simulation->getFileRegistryService()->stop();
        return 0;
    }
};

TEST_F(BatchServiceTest, BestFitStandardJobSubmissionTest) {
    DO_TEST_WITH_FORK(do_BestFitTaskTest_test);
}

void BatchServiceTest::do_BestFitTaskTest_test() {

    // Create and initialize a simulation
    wrench::Simulation *simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("batch_service_test");

    EXPECT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = simulation->getHostnameList()[0];

    // Create a WMS
    EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
            std::unique_ptr<wrench::WMS>(new BestFitStandardJobSubmissionTestWMS(this, workflow,
                                                                                   std::unique_ptr<wrench::Scheduler>(
                            new NoopScheduler()), hostname))));

    // Create a Storage Service
    EXPECT_NO_THROW(storage_service1 = simulation->add(
            std::unique_ptr<wrench::SimpleStorageService>(
                    new wrench::SimpleStorageService(hostname, 10000000000000.0))));

    // Create a Storage Service
    EXPECT_NO_THROW(storage_service2 = simulation->add(
            std::unique_ptr<wrench::SimpleStorageService>(
                    new wrench::SimpleStorageService(hostname, 10000000000000.0))));

    // Create a Batch Service
    EXPECT_NO_THROW(compute_service = simulation->add(
            std::unique_ptr<wrench::BatchService>(
                    new wrench::BatchService(hostname,simulation->getHostnameList(),
                                             storage_service1,true,true,{{wrench::StandardJobExecutorProperty::HOST_SELECTION_ALGORITHM, "BESTFIT"}}))));

    std::unique_ptr<wrench::FileRegistryService> file_registry_service(
            new wrench::FileRegistryService(hostname));

    simulation->setFileRegistryService(std::move(file_registry_service));

    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
    wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);
    wrench::WorkflowFile *input_file_1 = this->workflow->addFile("input_file_1", 10000.0);
    wrench::WorkflowFile *output_file_1 = this->workflow->addFile("output_file_1", 20000.0);
    wrench::WorkflowFile *input_file_2 = this->workflow->addFile("input_file_2", 10000.0);
    wrench::WorkflowFile *output_file_2 = this->workflow->addFile("output_file_2", 20000.0);

    // Staging the input_file on the storage service
    EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service1));
    EXPECT_NO_THROW(simulation->stageFiles({input_file_1}, storage_service1));
    EXPECT_NO_THROW(simulation->stageFiles({input_file_2}, storage_service1));


    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    EXPECT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}