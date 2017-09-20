//
// Created by suraj on 9/10/17.
//

#include "BatchServiceTest.h"

#include <wrench-dev.h>
#include <simgrid_S4U_util/S4U_Mailbox.h>
#include <simulation/SimulationMessage.h>
#include <services/compute_services/standard_job_executor/StandardJobExecutorMessage.h>

#include "TestWithFork.h"


class BatchServiceTest : public ::testing::Test {

public:
    wrench::StorageService *storage_service1 = nullptr;
    wrench::StorageService *storage_service2 = nullptr;
    wrench::ComputeService *compute_service = nullptr;
    wrench::Simulation *simulation;


    void do_StandardJobTaskTest_test();


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

        // Create a sequential task that lasts one hour and requires 12 cores
        wrench::WorkflowTask *task = this->workflow->addTask("task", 3600, 2, 2, 1.0);
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


        std::string my_mailbox = "test_callback_mailbox";

        std::map<std::string,unsigned long> batch_job_args;
        batch_job_args["-N"] = 1;
        batch_job_args["-t"] = 1; //time in minutes
        batch_job_args["-c"] = 4; //number of cores per node
        job_manager->submitJob(job,this->test->compute_service,batch_job_args);
        workflow->removeTask(task);

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
                            new wrench::RandomScheduler()), hostname))));

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
#include <services/batch_service/BatchService.h>