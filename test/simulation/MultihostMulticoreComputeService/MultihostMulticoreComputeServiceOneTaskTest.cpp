/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <math.h>
#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../include/TestWithFork.h"


class MultihostMulticoreComputeServiceOneTaskTest : public ::testing::Test {

public:
    wrench::WorkflowFile *input_file;
    wrench::WorkflowFile *output_file;
    wrench::WorkflowTask *task;
    wrench::StorageService *storage_service1 = nullptr;
    wrench::StorageService *storage_service2 = nullptr;
    wrench::ComputeService *compute_service = nullptr;

    void do_BadSetup_test();

    void do_Noop_test();

    void do_StandardJobConstructor_test();

    void do_HostMemory_test();

    void do_ExecutionWithLocationMap_test();

    void do_ExecutionWithDefaultStorageService_test();

    void do_ExecutionWithPrePostCopies_test();

    void do_ExecutionWithMissingFile_test();

    void do_ExecutionWithNotEnoughCores_test();

    void do_ExecutionWithNotEnoughRAM_test();

    void do_ExecutionWithDownService_test();


protected:
    MultihostMulticoreComputeServiceOneTaskTest() {

      // Create the simplest workflow
      workflow = new wrench::Workflow();

      // Create two files
      input_file = workflow->addFile("input_file", 10000.0);
      output_file = workflow->addFile("output_file", 20000.0);

      // Create one task
      task = workflow->addTask("task", 3600, 1, 1, 1.0, 0);
      task->addInputFile(input_file);
      task->addOutputFile(output_file);

      // Create a one-host platform file
      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4.1\"> "
              "   <zone id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"SingleHost\" speed=\"1f\" core=\"2\"/> "
              "       <host id=\"OneCoreHost\" speed=\"1f\" core=\"1\"/> "
              "       <host id=\"RAMHost\" speed=\"1f\" core=\"1\" > "
              "         <prop id=\"ram\" value=\"1024\" />"
              "       </host> "
              "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
              "       <route src=\"SingleHost\" dst=\"OneCoreHost\"> <link_ctn id=\"1\"/> </route>"
              "       <route src=\"RAMHost\" dst=\"OneCoreHost\"> <link_ctn id=\"1\"/> </route>"
              "   </zone> "
              "</platform>";
      FILE *platform_file = fopen(platform_file_path.c_str(), "w");
      fprintf(platform_file, "%s", xml.c_str());
      fclose(platform_file);

    }

    std::string platform_file_path = "/tmp/platform.xml";
    wrench::Workflow *workflow;

};


/**********************************************************************/
/**  BAD SETUP SIMULATION TEST                                       **/
/**********************************************************************/

class BadSetupTestWMS : public wrench::WMS {

public:
    BadSetupTestWMS(MultihostMulticoreComputeServiceOneTaskTest *test,
                    const std::set<wrench::ComputeService *> &compute_services,
                    const std::set<wrench::StorageService *> &storage_services,
                    std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    MultihostMulticoreComputeServiceOneTaskTest *test;

    int main() {

      return 0;
    }
};

TEST_F(MultihostMulticoreComputeServiceOneTaskTest, BadSetup) {
  DO_TEST_WITH_FORK(do_BadSetup_test);
}

void MultihostMulticoreComputeServiceOneTaskTest::do_BadSetup_test() {


  // Create and initialize a simulation
  auto *simulation = new wrench::Simulation();

  int argc = 0;
  auto **argv = (char **) calloc(1, sizeof(char *));

  ASSERT_THROW(simulation->init(&argc, argv), std::invalid_argument);
  free(argv);

  argc = 1;
  ASSERT_THROW(simulation->init(&argc, nullptr), std::invalid_argument);

  ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

  ASSERT_THROW(simulation->launch(), std::runtime_error);


  argc = 1;
  argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Bad hostname
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService("bogus",
                                                       {std::make_tuple("bogus",
                                                                        wrench::ComputeService::ALL_CORES,
                                                                        wrench::ComputeService::ALL_RAM)},
                                                       {})), std::invalid_argument);

  // Get a hostname
  auto hostname = simulation->getHostnameList()[0];

  // Bad resource hostname
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple("bogus",
                                                                        wrench::ComputeService::ALL_CORES,
                                                                        wrench::ComputeService::ALL_RAM)},
                                                       {})), std::invalid_argument);

  // Bad number of cores
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple(hostname,
                                                                        0,
                                                                        wrench::ComputeService::ALL_RAM)},
                                                       {})), std::invalid_argument);

  // Bad number of cores
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple(hostname,
                                                                        100,
                                                                        wrench::ComputeService::ALL_RAM)},
                                                       {})), std::invalid_argument);

  // Bad RAM
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple("RAMHost",
                                                                        wrench::ComputeService::ALL_CORES,
                                                                        -1.0)},
                                                       {})), std::invalid_argument);

  // Bad RAM
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple("RAMHost",
                                                                        wrench::ComputeService::ALL_CORES,
                                                                        100000.0)},
                                                       {})), std::invalid_argument);


  // Bad PROPERTIES
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple("RAMHost",
                                                                        wrench::ComputeService::ALL_CORES,
                                                                        100000.0)},
                                                       0,
                                                       {
                                                               std::make_pair(wrench::MultihostMulticoreComputeServiceProperty::THREAD_STARTUP_OVERHEAD, "-1.0")
                                                       },
                                                       {})), std::invalid_argument);

  ASSERT_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple("RAMHost",
                                                                        wrench::ComputeService::ALL_CORES,
                                                                        100000.0)},
                                                       0,
                                                       {
                                                               std::make_pair(wrench::MultihostMulticoreComputeServiceProperty::RESOURCE_ALLOCATION_POLICY, "bogus")
                                                       },
                                                       {})), std::invalid_argument);

  ASSERT_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple("RAMHost",
                                                                        wrench::ComputeService::ALL_CORES,
                                                                        100000.0)},
                                                       0,
                                                       {
                                                               std::make_pair(wrench::MultihostMulticoreComputeServiceProperty::JOB_SELECTION_POLICY, "bogus")
                                                       },
                                                       {})), std::invalid_argument);

  ASSERT_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple("RAMHost",
                                                                        wrench::ComputeService::ALL_CORES,
                                                                        100000.0)},
                                                       0,
                                                       {
                                                               std::make_pair(wrench::MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_CORE_ALLOCATION_ALGORITHM, "bogus")
                                                       },
                                                       {})), std::invalid_argument);

  ASSERT_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple("RAMHost",
                                                                        wrench::ComputeService::ALL_CORES,
                                                                        100000.0)},
                                                       0,
                                                       {
                                                               std::make_pair(wrench::MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_HOST_SELECTION_ALGORITHM, "bogus")
                                                       },
                                                       {})), std::invalid_argument);

  ASSERT_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple("RAMHost",
                                                                        wrench::ComputeService::ALL_CORES,
                                                                        100000.0)},
                                                       0,
                                                       {
                                                               std::make_pair(wrench::MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_TASK_SELECTION_ALGORITHM, "bogus")
                                                       },
                                                       {})), std::invalid_argument);
  

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(new BadSetupTestWMS(this,
                                                            {}, {}, hostname)));

  ASSERT_THROW(wms->addWorkflow(nullptr), std::invalid_argument);
  ASSERT_NO_THROW(wms->addWorkflow(this->workflow));
  ASSERT_THROW(wms->addWorkflow(this->workflow), std::invalid_argument);

  // Running a "run a single task" simulation
  ASSERT_THROW(simulation->launch(), std::runtime_error);

  delete simulation;

  free(argv[0]);
  free(argv);
}

/**********************************************************************/
/**  NOOP SIMULATION TEST                                            **/
/**********************************************************************/

class NoopTestWMS : public wrench::WMS {

public:
    NoopTestWMS(MultihostMulticoreComputeServiceOneTaskTest *test,
                const std::set<wrench::ComputeService *> &compute_services,
                const std::set<wrench::StorageService *> &storage_services,
                std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    MultihostMulticoreComputeServiceOneTaskTest *test;

    int main() {

      wrench::TerminalOutput::disableColor(); // just for increasing stupid coverage

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      // Create a data movement manager
      std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      // Stop the Job Manager manually, just for kicks
      job_manager->stop();

      // Stop the Data Movement Manager manually, just for kicks
      data_movement_manager->stop();

      return 0;
    }
};

TEST_F(MultihostMulticoreComputeServiceOneTaskTest, Noop) {
  DO_TEST_WITH_FORK(do_Noop_test);
}

void MultihostMulticoreComputeServiceOneTaskTest::do_Noop_test() {


  // Create and initialize a simulation
  auto *simulation = new wrench::Simulation();


  int argc = 1;
  auto argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");


  ASSERT_NO_THROW(simulation->init(&argc, argv));


  // Setting up the platform
  ASSERT_THROW(simulation->launch(), std::runtime_error);
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
  ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

  ASSERT_THROW(simulation->add((wrench::ComputeService *)nullptr), std::invalid_argument);

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Compute Service
  ASSERT_THROW(simulation->launch(), std::runtime_error);
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},100.0,
                                                       {})));

  // Create a Storage Service
  ASSERT_THROW(simulation->launch(), std::runtime_error);
  ASSERT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a WMS
  ASSERT_THROW(simulation->launch(), std::runtime_error);
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new NoopTestWMS(
                  this,
                  { compute_service }, {
                          storage_service1
                  }, hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  // Without a file registry service this should fail
  ASSERT_THROW(simulation->launch(), std::runtime_error);
  ASSERT_THROW(simulation->stageFiles({{input_file->getID(),input_file}}, storage_service1), std::runtime_error);

  simulation->add(new wrench::FileRegistryService(hostname));

  ASSERT_THROW(simulation->stageFiles({{input_file->getID(), input_file}}, nullptr), std::invalid_argument);
  ASSERT_THROW(simulation->stageFiles({{"foo", nullptr}}, storage_service1), std::invalid_argument);

  // Staging the input_file on the storage service
  ASSERT_NO_THROW(simulation->stageFiles({{input_file->getID(), input_file}}, storage_service1));

  // Running a "do nothing" simulation
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}


/**********************************************************************/
/**  STANDARD JOB CONSTRUCTOR TEST                                   **/
/**********************************************************************/

class StandardJobConstructorTestWMS : public wrench::WMS {

public:
    StandardJobConstructorTestWMS(
            MultihostMulticoreComputeServiceOneTaskTest *test,
            const std::set<wrench::ComputeService *> &compute_services,
            const std::set<wrench::StorageService *> &storage_services,
            std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    MultihostMulticoreComputeServiceOneTaskTest *test;


    int main() {

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      bool success;

      wrench::StandardJob *job = nullptr;

      // Create a job with nullptr task (and no file copies)
      success = true;
      try {
        job = job_manager->createStandardJob(nullptr,
                                             {{test->input_file,  test->storage_service1},
                                              {test->output_file, test->storage_service1}});
      } catch (std::invalid_argument &e) {
        success = false;
      }

      if (success) {
        throw std::runtime_error("Should not be able to create a job with an empty task");
      }

      // Create a job with an empty vector of tasks (and no file copies)
      success = true;
      try {
        job = job_manager->createStandardJob({},
                                             {{test->input_file,  test->storage_service1},
                                              {test->output_file, test->storage_service1}});
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a job with an empty task vector");
      }

      // Create a job with a vector of empty tasks (and no file copies)
      success = true;
      try {
        job = job_manager->createStandardJob({nullptr, nullptr},
                                             {{test->input_file,  test->storage_service1},
                                              {test->output_file, test->storage_service1}});
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a job with a vector of empty tasks");
      }

      // Create another task
        wrench::WorkflowTask *task_big = this->getWorkflow()->addTask("task2", 3600, 2, 2, 1.0, 2048);

      // Create a job with nullptrs in file locations
      success = true;
      try {

        job = job_manager->createStandardJob({test->task, task_big},
                                             {{nullptr,  test->storage_service1},
                                              {test->output_file, test->storage_service1}});
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a job with an nullptr file in file locations");
      }

      // Create a job with nullptrs in file locations
      success = true;
      try {
        job = job_manager->createStandardJob({test->task, task_big},
                                             {{test->input_file,  nullptr},
                                              {test->output_file, test->storage_service1}});
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a job with an nullptr storage service in file locations");
      }

      // Create a job with nullptrs in pre file copies
      success = true;
      try {
        job = job_manager->createStandardJob({test->task},
                                             {{test->input_file,  test->storage_service1},
                                              {test->output_file, test->storage_service1}},
                                             {std::make_tuple(nullptr, test->storage_service1, test->storage_service2)
                                             },
                                             {},
                                             {}
        );
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a job with a nullptr file in pre file copies");
      }

      // Create a job with nullptrs in pre file copies
      success = true;
      try {
        job = job_manager->createStandardJob({test->task},
                                             {{test->input_file,  test->storage_service1},
                                              {test->output_file, test->storage_service1}},
                                             {std::make_tuple(test->output_file, nullptr, test->storage_service2)
                                             },
                                             {},
                                             {}
        );
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a job with a nullptr src storage service in pre file copies");
      }

      // Create a job with nullptrs in pre file copies
      success = true;
      try {
        job = job_manager->createStandardJob({test->task},
                                             {{test->input_file,  test->storage_service1},
                                              {test->output_file, test->storage_service1}},
                                             {std::make_tuple(test->output_file, test->storage_service1, nullptr)},
                                             {},
                                             {}
        );
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a job with a nullptr dst storage service in pre file copies");
      }

      // Create a job with nullptrs in post file copies
      success = true;
      try {
        job = job_manager->createStandardJob({test->task},
                                             {{test->input_file,  test->storage_service1},
                                              {test->output_file, test->storage_service1}},
                                             {},
                                             {std::make_tuple(nullptr, test->storage_service1, test->storage_service2)},
                                             {}
        );
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a job with a nullptr file in post file copies");
      }

      // Create a job with nullptrs in post file copies
      success = true;
      try {
        job = job_manager->createStandardJob({test->task},
                                             {{test->input_file,  test->storage_service1},
                                              {test->output_file, test->storage_service1}},
                                             {},
                                             {std::make_tuple(test->output_file, nullptr, test->storage_service2)},
                                             {}
        );
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a job with a nullptr src storage service in post file copies");
      }

      // Create a job with nullptrs in post file copies
      success = true;
      try {
        job = job_manager->createStandardJob({test->task},
                                             {{test->input_file,  test->storage_service1},
                                              {test->output_file, test->storage_service1}},
                                             {},
                                             {std::make_tuple(test->output_file, test->storage_service1, nullptr)
                                             },
                                             {}
        );
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a job with a nullptr dst storage service in post file copies");
      }

      // Create a job with nullptrs in post file deletions
      success = true;
      try {
        job = job_manager->createStandardJob({test->task},
                                             {{test->input_file,  test->storage_service1},
                                              {test->output_file, test->storage_service1}},
                                             {},
                                             {},
                                             {std::make_tuple(nullptr, test->storage_service1)}
        );
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a job with a nullptr file in file deletions");
      }


      // Create a job with nullptrs in post file deletions
      success = true;
      try {
        job = job_manager->createStandardJob({test->task},
                                             {{test->input_file,  test->storage_service1},
                                              {test->output_file, test->storage_service1}},
                                             {},
                                             {},
                                             {std::make_tuple(test->input_file, nullptr)}
        );
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a job with a nullptr storage service in file deletions");
      }



      // Stop the Job Manager manually, just for kicks
      job_manager->stop();

      return 0;
    }
};


TEST_F(MultihostMulticoreComputeServiceOneTaskTest, StandardJobConstructor) {
  DO_TEST_WITH_FORK(do_StandardJobConstructor_test);
}

void MultihostMulticoreComputeServiceOneTaskTest::do_StandardJobConstructor_test() {

  // Create and initialize a simulation
  auto *simulation = new wrench::Simulation();
  int argc = 1;
  auto **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  simulation->instantiatePlatform(platform_file_path);

  // Get a hostname
  std::string hostname1 = "SingleHost";

  // Create a Compute Service
  compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname1,
                                                       {std::make_tuple(hostname1, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},100.0,
                                                       {}));

  // Create a Storage Service
  storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname1, 10000000000000.0));

  // Start a file registry service
  simulation->add(new wrench::FileRegistryService(hostname1));

  // Create a WMS
  wrench::WMS *wms = simulation->add(
          new StandardJobConstructorTestWMS(
                  this, {compute_service}, {storage_service1}, hostname1));

  wms->addWorkflow(workflow);

  // Staging the input_file on the storage service
  simulation->stageFiles({{input_file->getID(), input_file}}, storage_service1);

  // Running a "do nothing" simulation
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);


}

/**********************************************************************/
/**  HOST MEMORY SIMULATION TEST                                     **/
/**********************************************************************/

class HostMemoryTestWMS : public wrench::WMS {

public:
    HostMemoryTestWMS(MultihostMulticoreComputeServiceOneTaskTest *test,
                      const std::set<wrench::ComputeService *> &compute_services,
                      const std::set<wrench::StorageService *> &storage_services,
                      std::string &hostname1,
                      std::string &hostname2) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname1, "test") {
      this->test = test;
    }


private:

    MultihostMulticoreComputeServiceOneTaskTest *test;

    int main() {


      double ram_capacity;

      ram_capacity = wrench::Simulation::getHostMemoryCapacity("SingleHost");
      if (ram_capacity != wrench::ComputeService::ALL_RAM) {
        throw std::runtime_error("RAM Capacity of SingleHost should be +infty");
      }

      ram_capacity = wrench::Simulation::getMemoryCapacity();
      if (ram_capacity != wrench::ComputeService::ALL_RAM) {
        throw std::runtime_error("RAM Capacity of SingleHost should be +infty");
      }


      ram_capacity = wrench::Simulation::getHostMemoryCapacity("RAMHost");
      if (ram_capacity == wrench::ComputeService::ALL_RAM) {
        throw std::runtime_error("RAM Capacity of RAMHost should not be +infty");
      }
      if (fabs(ram_capacity - 1024) > 0.01) {
        throw std::runtime_error("RAM Capacity of RAMHost should  be 1024");
      }


      return 0;
    }
};

TEST_F(MultihostMulticoreComputeServiceOneTaskTest, HostMemory) {
  DO_TEST_WITH_FORK(do_HostMemory_test);
}

void MultihostMulticoreComputeServiceOneTaskTest::do_HostMemory_test() {


  // Create and initialize a simulation
  auto *simulation = new wrench::Simulation();
  int argc = 1;
  auto **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  simulation->instantiatePlatform(platform_file_path);

  // Get a hostname
  std::string hostname1 = "SingleHost";
  std::string hostname2 = "RAMHost";

  // Create a Compute Service
  compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname1,
                                                       {std::make_tuple(hostname1, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},100.0,
                                                       {}));

  // Create a Storage Service
  storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname1, 10000000000000.0));

  // Start a file registry service
  simulation->add(new wrench::FileRegistryService(hostname1));

  // Create a WMS
  wrench::WMS *wms = simulation->add(
          new HostMemoryTestWMS(
                  this,  {compute_service}, {storage_service1}, hostname1, hostname2));

  wms->addWorkflow(workflow);

  // Staging the input_file on the storage service
  simulation->stageFiles({{input_file->getID(), input_file}}, storage_service1);

  // Running a "do nothing" simulation
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}


/**********************************************************************/
/** EXECUTION WITH LOCATION_MAP SIMULATION TEST                      **/
/**********************************************************************/

class ExecutionWithLocationMapTestWMS : public wrench::WMS {

public:
    ExecutionWithLocationMapTestWMS(MultihostMulticoreComputeServiceOneTaskTest *test,
                                    const std::set<wrench::ComputeService *> &compute_services,
                                    const std::set<wrench::StorageService *> &storage_services,
                                    std::string &hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    MultihostMulticoreComputeServiceOneTaskTest *test;

    int main() {

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      wrench::StandardJob *job = nullptr;

      bool success;


      // Create a job
      job = job_manager->createStandardJob(test->task,
                                           {{test->input_file,  test->storage_service1},
                                            {test->output_file, test->storage_service1}});

      // Get the job type as a string
      std::string job_type_as_string = job->getTypeAsString();
      if (job_type_as_string != "Standard") {
        throw std::runtime_error("Job type as a string should be 'Standard'");
      }

      // Submit the job
      job_manager->submitJob(job, test->compute_service);

      // Wait for the workflow execution event
      std::unique_ptr<wrench::WorkflowExecutionEvent> event = this->getWorkflow()->waitForNextExecutionEvent();
      if (event->type != wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION) {
        throw std::runtime_error("Unexpected workflow execution event!");
      }

      if (!this->test->storage_service1->lookupFile(this->test->output_file, nullptr)) {
        throw std::runtime_error("Output file not written to storage service");
      }


      /* Do a bogus lookup of the file registry service */
      success = true;
      try {
        this->getAvailableFileRegistryService()->lookupEntry(nullptr);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Shouldn't be able to lookup a nullptr entry in the File Registry Service");
      }

      /* Do a bogus add entry of the file registry service */
      success = true;
      try {
        this->getAvailableFileRegistryService()->addEntry(nullptr, nullptr);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Shouldn't be able to add nullptr entry in the File Registry Service");
      }

      /* Do a bogus remove entry of the file registry service */
      success = true;
      try {
        this->getAvailableFileRegistryService()->removeEntry(nullptr, nullptr);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Shouldn't be able to remove nullptr entry in the File Registry Service");
      }

      return 0;
    }
};

TEST_F(MultihostMulticoreComputeServiceOneTaskTest, ExecutionWithLocationMap) {
  DO_TEST_WITH_FORK(do_ExecutionWithLocationMap_test);
}

void MultihostMulticoreComputeServiceOneTaskTest::do_ExecutionWithLocationMap_test() {

  // Create and initialize a simulation
  auto *simulation = new wrench::Simulation();
  int argc = 1;
  auto **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Compute Service
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {})));

  // Create a Storage Service
  ASSERT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a File Registry Service
  ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new ExecutionWithLocationMapTestWMS(
                  this,  {
                          compute_service
                  }, {
                          storage_service1
                  }, hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  // Staging the input_file on the storage service
  ASSERT_NO_THROW(simulation->stageFiles({{input_file->getID(), input_file}}, storage_service1));

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  // Check that the output trace makes sense
  ASSERT_EQ(task->getState(), wrench::WorkflowTask::COMPLETED);
  ASSERT_EQ(task->getFailureCount(), 0);
  ASSERT_GT(task->getStartDate(), 0.0);
  ASSERT_GT(task->getEndDate(), 0.0);
  ASSERT_GT(task->getEndDate(), task->getStartDate());
  ASSERT_EQ(workflow->getCompletionDate(), task->getEndDate());

  std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampTaskCompletion> *> task_completion_trace =
          simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();
  ASSERT_EQ(simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>().size(), 1);
  ASSERT_EQ(simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getContent()->getDate(),
            task->getEndDate());
  ASSERT_EQ(simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getContent()->getTask(),
            task);


  delete simulation;

  free(argv[0]);
  free(argv);
}

/**********************************************************************/
/** EXECUTION WITH DEFAULT STORAGE SERVICE SIMULATION TEST           **/
/**********************************************************************/

class ExecutionWithDefaultStorageServiceTestWMS : public wrench::WMS {

public:
    ExecutionWithDefaultStorageServiceTestWMS(MultihostMulticoreComputeServiceOneTaskTest *test,
                                              const std::set<wrench::ComputeService *> &compute_services,
                                              const std::set<wrench::StorageService *> &storage_services,
                                              std::string &hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    MultihostMulticoreComputeServiceOneTaskTest *test;

    int main() {

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      // Create a job
      wrench::StandardJob *job = job_manager->createStandardJob(test->task,
                                                                {});

      // Submit the job
      job_manager->submitJob(job, test->compute_service);

      // Wait for the workflow execution event
      std::unique_ptr<wrench::WorkflowExecutionEvent> event = this->getWorkflow()->waitForNextExecutionEvent();
      if (event->type != wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION) {
        throw std::runtime_error("Unexpected workflow execution event!");
      }

      if (!this->test->storage_service1->lookupFile(this->test->output_file, job)) {
        throw std::runtime_error("Output file not written to storage service");
      }

      return 0;
    }
};

TEST_F(MultihostMulticoreComputeServiceOneTaskTest, DISABLED_ExecutionWithDefaultStorageService) {

  DO_TEST_WITH_FORK(do_ExecutionWithDefaultStorageService_test);
}

void MultihostMulticoreComputeServiceOneTaskTest::do_ExecutionWithDefaultStorageService_test() {
  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  ASSERT_THROW(simulation->launch(), std::runtime_error);

  simulation->init(&argc, argv);

  // Setting up the platform
  ASSERT_THROW(simulation->launch(), std::runtime_error);
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
  ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Storage Service
  ASSERT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Compute Service
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new ExecutionWithDefaultStorageServiceTestWMS(
                  this,  {compute_service},
                  { storage_service1 }, hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  // Create a File Registry Service
  ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

  // Staging the input_file on the storage service
  ASSERT_NO_THROW(simulation->stageFiles({{input_file->getID(), input_file}}, storage_service1));


  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  // Check that the output trace makes sense
  ASSERT_EQ(task->getState(), wrench::WorkflowTask::COMPLETED);
  ASSERT_EQ(task->getFailureCount(), 0);
  ASSERT_GT(task->getStartDate(), 0.0);
  ASSERT_GT(task->getEndDate(), 0.0);
  ASSERT_GT(task->getEndDate(), task->getStartDate());

  std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampTaskCompletion> *> task_completion_trace =
          simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();
  ASSERT_EQ(simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>().size(), 1);
  ASSERT_EQ(simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getContent()->getDate(),
            task->getEndDate());
  ASSERT_EQ(simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getContent()->getTask(),
            task);

  delete simulation;

  free(argv[0]);
  free(argv);
}

/**********************************************************************/
/** EXECUTION WITH PRE/POST COPIES AND CLEANUP SIMULATION TEST       **/
/**********************************************************************/

class ExecutionWithPrePostCopiesAndCleanupTestWMS : public wrench::WMS {

public:
    ExecutionWithPrePostCopiesAndCleanupTestWMS(MultihostMulticoreComputeServiceOneTaskTest *test,
                                                const std::set<wrench::ComputeService *> &compute_services,
                                                const std::set<wrench::StorageService *> &storage_services,
                                                std::string &hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    MultihostMulticoreComputeServiceOneTaskTest *test;

    int main() {

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      // Create a job
      wrench::StandardJob *job = job_manager->createStandardJob({test->task},
                                                                {{test->input_file,test->storage_service1},{test->output_file,test->storage_service2}}, //changed this since we don't have default storage now
                                                                {std::make_tuple(test->input_file, test->storage_service1, test->storage_service2)},
                                                                {std::make_tuple(test->output_file, test->storage_service2, test->storage_service1)},
                                                                {std::make_tuple(test->input_file, test->storage_service2),
                                                                 std::make_tuple(test->output_file, test->storage_service2)});
      // Submit the job
      job_manager->submitJob(job, test->compute_service);

      // Wait for the workflow execution event
      std::unique_ptr<wrench::WorkflowExecutionEvent> event = this->getWorkflow()->waitForNextExecutionEvent();
      switch (event->type) {
        case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
          break;
        }
        case wrench::WorkflowExecutionEvent::STANDARD_JOB_FAILURE: {
          throw std::runtime_error("Unexpected job failure: " +
                                           dynamic_cast<wrench::StandardJobFailedEvent*>(event.get())->failure_cause->toString());
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string(event->type));
        }
      }

      // Test file locations
      if (!this->test->storage_service1->lookupFile(this->test->input_file, nullptr)) {
        throw std::runtime_error("Input file should be on Storage Service #1");
      }
      if (!this->test->storage_service1->lookupFile(this->test->output_file, nullptr)) {
        throw std::runtime_error("Output file should be on Storage Service #1");
      }
      if (this->test->storage_service2->lookupFile(this->test->input_file,job)) {
        throw std::runtime_error("Input file should not be on Storage Service #2");
      }
      if (this->test->storage_service2->lookupFile(this->test->input_file,job)) {
        throw std::runtime_error("Output file should not be on Storage Service #2");
      }

      return 0;
    }
};

TEST_F(MultihostMulticoreComputeServiceOneTaskTest, ExecutionWithPrePostCopies) {
  DO_TEST_WITH_FORK(do_ExecutionWithPrePostCopies_test)
}

void MultihostMulticoreComputeServiceOneTaskTest::do_ExecutionWithPrePostCopies_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  ASSERT_THROW(simulation->launch(), std::runtime_error);

  simulation->init(&argc, argv);

  // Setting up the platform
  ASSERT_THROW(simulation->launch(), std::runtime_error);
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
  ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Storage Service
  ASSERT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create another Storage Service
  ASSERT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));


  // Create a Compute Service with default Storage Service #2
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new ExecutionWithPrePostCopiesAndCleanupTestWMS(this,
                                                          { compute_service }, {
                                                                  storage_service1, storage_service2
                                                          }, hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  // Create a File Registry Service
  ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

  // Staging the input_file on storage service #1
  ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  // Check that the output trace makes sense
  ASSERT_EQ(task->getState(), wrench::WorkflowTask::COMPLETED);
  ASSERT_EQ(task->getFailureCount(), 0);
  ASSERT_GT(task->getStartDate(), 0.0);
  ASSERT_GT(task->getEndDate(), 0.0);
  ASSERT_GT(task->getEndDate(), task->getStartDate());

  std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampTaskCompletion> *> task_completion_trace =
          simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();
  ASSERT_EQ(simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>().size(), 1);
  ASSERT_EQ(simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getContent()->getDate(),
            task->getEndDate());
  ASSERT_EQ(simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getContent()->getTask(),
            task);

  delete simulation;

  free(argv[0]);
  free(argv);
}

/********************************************************/
/** EXECUTION WITH MISSING FILE  SIMULATION TEST       **/
/********************************************************/

class ExecutionWithMissingFileTestWMS : public wrench::WMS {

public:
    ExecutionWithMissingFileTestWMS(MultihostMulticoreComputeServiceOneTaskTest *test,
                                    const std::set<wrench::ComputeService *> &compute_services,
                                    const std::set<wrench::StorageService *> &storage_services,
                                    std::string &hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    MultihostMulticoreComputeServiceOneTaskTest *test;

    int main() {

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      // Remove the staged file!
      this->test->storage_service1->deleteFile(test->input_file);

      // Create a job
      wrench::StandardJob *job = job_manager->createStandardJob({test->task},
                                                                {},
                                                                {},
                                                                {},
                                                                {});
      // Submit the job
      job_manager->submitJob(job, test->compute_service);

      // Wait for the workflow execution event
      std::unique_ptr<wrench::WorkflowExecutionEvent> event = this->getWorkflow()->waitForNextExecutionEvent();
      switch (event->type) {
        case wrench::WorkflowExecutionEvent::STANDARD_JOB_FAILURE: {
          bool success = false;
          if (dynamic_cast<wrench::StandardJobFailedEvent*>(event.get())->failure_cause->getCauseType() != wrench::FailureCause::NO_STORAGE_SERVICE_FOR_FILE) {
            throw std::runtime_error(
                    "Got an Standard Job Failure as expected, but it does not have the correct failure cause type");
          }
          auto real_cause = (wrench::NoStorageServiceForFile *) dynamic_cast<wrench::StandardJobFailedEvent*>(event.get())->failure_cause.get();
          std::string error_msg = real_cause->toString();
          if (real_cause->getFile() != test->input_file) {
            throw std::runtime_error(
                    "Got the expected failure, but the failure cause does not point to the right file");
          }
          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string(event->type));
        }
      }

      return 0;
    }
};

TEST_F(MultihostMulticoreComputeServiceOneTaskTest, ExecutionWithMissingFile) {
  DO_TEST_WITH_FORK(do_ExecutionWithMissingFile_test)
}

void MultihostMulticoreComputeServiceOneTaskTest::do_ExecutionWithMissingFile_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  ASSERT_THROW(simulation->launch(), std::runtime_error);

  simulation->init(&argc, argv);

  // Setting up the platform
  ASSERT_THROW(simulation->launch(), std::runtime_error);
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
  ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Storage Service
  ASSERT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create another Storage Service
  ASSERT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));


  // Create a Compute Service with no default Storage Service
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new ExecutionWithMissingFileTestWMS(this,
                                              { compute_service }, {storage_service1, storage_service2
                                              }, hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  // Create a File Registry Service
  ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

  // Staging the input_file on storage service #1
  ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}



/********************************************************/
/** EXECUTION WITH NOT ENOUGH CORES SIMULATION TEST    **/
/********************************************************/

class ExecutionWithNotEnoughCoresTestWMS : public wrench::WMS {

public:
    ExecutionWithNotEnoughCoresTestWMS(MultihostMulticoreComputeServiceOneTaskTest *test,
                                       const std::set<wrench::ComputeService *> &compute_services,
                                       const std::set<wrench::StorageService *> &storage_services,
                                       std::string &hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    MultihostMulticoreComputeServiceOneTaskTest *test;

    int main() {

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();


      bool success;

      // Create another task
      wrench::WorkflowTask *task_big = this->getWorkflow()->addTask("task2", 3600, 2, 2, 1.0, 2048);

      // Create a job
      wrench::StandardJob *job = job_manager->createStandardJob({task_big},
                                                                {},
                                                                {}, {}, {});
      // Submit the job
      success = true;
      try {
        job_manager->submitJob(job, test->compute_service);
      } catch (wrench::WorkflowExecutionException &e) {
        if (e.getCause()->getCauseType() != wrench::FailureCause::NOT_ENOUGH_COMPUTE_RESOURCES) {
          throw std::runtime_error("Received the expected exception, but the failure cause is incorrect");
        }
        auto real_cause = (wrench::NotEnoughComputeResources *) e.getCause().get();
        std::string error_msg = real_cause->toString();
        if (real_cause->getJob() != job) {
          throw std::runtime_error(
                  "Got the expected failure, but the failure cause does not point to the right job");
        }
        if (real_cause->getComputeService() != test->compute_service) {
          throw std::runtime_error(
                  "Got the expected failure, but the failure cause does not point to the right compute service");
        }
        success = false;
      }

      if (success) {
        throw std::runtime_error("Should not be able to submit a job to a service without enough cores");
      }

      return 0;
    }
};

TEST_F(MultihostMulticoreComputeServiceOneTaskTest, ExecutionWithNotEnoughCores) {
  DO_TEST_WITH_FORK(do_ExecutionWithNotEnoughCores_test)
}

void MultihostMulticoreComputeServiceOneTaskTest::do_ExecutionWithNotEnoughCores_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  ASSERT_THROW(simulation->launch(), std::runtime_error);

  simulation->init(&argc, argv);

  // Setting up the platform
  ASSERT_THROW(simulation->launch(), std::runtime_error);
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
  ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Storage Service
  ASSERT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create another Storage Service
  ASSERT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));


  // Create a Compute Service with no default Storage Service
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService("OneCoreHost",
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new ExecutionWithNotEnoughCoresTestWMS(this,
                                                 { compute_service }, {storage_service1, storage_service2
                                                 }, hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  // Create a File Registry Service
  ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

  // Staging the input_file on storage service #1
  ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}



/********************************************************/
/** EXECUTION WITH NOT ENOUGH RAM SIMULATION TEST      **/
/********************************************************/

class ExecutionWithNotEnoughRAMTestWMS : public wrench::WMS {

public:
    ExecutionWithNotEnoughRAMTestWMS(MultihostMulticoreComputeServiceOneTaskTest *test,
                                     const std::set<wrench::ComputeService *> &compute_services,
                                     const std::set<wrench::StorageService *> &storage_services,
                                     std::string &hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    MultihostMulticoreComputeServiceOneTaskTest *test;

    int main() {

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();


      bool success;

      wrench::WorkflowTask *task_big = this->getWorkflow()->addTask("task2", 3600, 2, 2, 1.0, 2048);

      // Create a job
      wrench::StandardJob *job = job_manager->createStandardJob({task_big},
                                                                {},
                                                                {}, {}, {});
      // Submit the job
      success = true;
      try {
        job_manager->submitJob(job, test->compute_service);
      } catch (wrench::WorkflowExecutionException &e) {
        if (e.getCause()->getCauseType() != wrench::FailureCause::NOT_ENOUGH_COMPUTE_RESOURCES) {
          throw std::runtime_error("Received the expected exception, but the failure cause is incorrect");
        }
        auto real_cause = (wrench::NotEnoughComputeResources *) e.getCause().get();
        std::string error_msg = real_cause->toString();
        if (real_cause->getJob() != job) {
          throw std::runtime_error(
                  "Got the expected failure, but the failure cause does not point to the right job");
        }
        if (real_cause->getComputeService() != test->compute_service) {
          throw std::runtime_error(
                  "Got the expected failure, but the failure cause does not point to the right compute service");
        }
        success = false;
      }

      if (success) {
        throw std::runtime_error("Should not be able to submit a job to a service without enough cores");
      }

      return 0;
    }
};

TEST_F(MultihostMulticoreComputeServiceOneTaskTest, ExecutionWithNotEnoughRAM) {
  DO_TEST_WITH_FORK(do_ExecutionWithNotEnoughRAM_test)
}

void MultihostMulticoreComputeServiceOneTaskTest::do_ExecutionWithNotEnoughRAM_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  ASSERT_THROW(simulation->launch(), std::runtime_error);

  simulation->init(&argc, argv);

  // Setting up the platform
  ASSERT_THROW(simulation->launch(), std::runtime_error);
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
  ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Storage Service
  ASSERT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create another Storage Service
  ASSERT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));


  // Create a Compute Service with no default Storage Service
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService("RAMHost",
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new ExecutionWithNotEnoughRAMTestWMS(this,
                                               { compute_service }, {storage_service1, storage_service2
                                               }, hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  // Create a File Registry Service
  ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

  // Staging the input_file on storage service #1
  ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}



/**********************************************************************/
/** EXECUTION WITH DOWN SERVICE SIMULATION TEST                      **/
/**********************************************************************/

class ExecutionWithDownServiceTestWMS : public wrench::WMS {

public:
    ExecutionWithDownServiceTestWMS(MultihostMulticoreComputeServiceOneTaskTest *test,
                                    const std::set<wrench::ComputeService *> &compute_services,
                                    const std::set<wrench::StorageService *> &storage_services,
                                    std::string &hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    MultihostMulticoreComputeServiceOneTaskTest *test;

    int main() {

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      wrench::StandardJob *job = nullptr;

      bool success;

      // Shutdown the service
      test->compute_service->stop();

      // Create a job
      job = job_manager->createStandardJob(test->task,
                                           {{test->input_file,  test->storage_service1},
                                            {test->output_file, test->storage_service1}});

      // Submit the job
      success = true;
      try {
        job_manager->submitJob(job, test->compute_service);
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
        if (e.getCause()->getCauseType() != wrench::FailureCause::SERVICE_DOWN) {
          throw std::runtime_error("Got the expected exception, but the failure cause is wrong");
        }
        auto real_cause = (wrench::ServiceIsDown *) e.getCause().get();
        std::string error_msg = real_cause->toString();
        if (real_cause->getService() != test->compute_service) {
          throw std::runtime_error(
                  "Got the expected failure, but the failure cause does not point to the right compute service");
        }
      }

      if (success) {
        throw std::runtime_error("Should not be able to submit a job to a service that is down");
      }

      return 0;
    }
};

TEST_F(MultihostMulticoreComputeServiceOneTaskTest, ExecutionWithDownService) {
  DO_TEST_WITH_FORK(do_ExecutionWithDownService_test);
}

void MultihostMulticoreComputeServiceOneTaskTest::do_ExecutionWithDownService_test() {

  // Create and initialize a simulation
  auto *simulation = new wrench::Simulation();
  int argc = 1;
  auto **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Compute Service
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {})));

  // Create a Storage Service
  ASSERT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a File Registry Service
  ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new ExecutionWithDownServiceTestWMS(
                  this,  {
                          compute_service
                  }, {
                          storage_service1
                  }, hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  // Staging the input_file on the storage service
  ASSERT_NO_THROW(simulation->stageFiles({{input_file->getID(), input_file}}, storage_service1));

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}