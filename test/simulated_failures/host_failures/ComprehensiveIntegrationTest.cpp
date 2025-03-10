/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"
#include "../failure_test_util/ResourceSwitcher.h"
#include "../failure_test_util/ResourceRandomRepeatSwitcher.h"


WRENCH_LOG_CATEGORY(comprehensive_integration_host_failure_test, "Log category for ComprehensiveIntegrationHostFailuresTest");

#define NUM_TASKS 100
#define MAX_TASK_DURATION_WITH_ON_CORE 3600
#define CHAOS_MONKEY_MIN_SLEEP_BEFORE_OFF 100
#define CHAOS_MONKEY_MAX_SLEEP_BEFORE_OFF 3000// The bigger this number the less flaky the platform
#define CHAOS_MONKEY_MIN_SLEEP_BEFORE_ON 100
#define CHAOS_MONKEY_MAX_SLEEP_BEFORE_ON 2000// The bigger this number the more flaky the platform

class ComprehensiveIntegrationHostFailuresTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::Workflow> workflow;
    std::map<std::string, bool> faulty_map;

    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service2 = nullptr;
    std::shared_ptr<wrench::CloudComputeService> cloud_service = nullptr;
    std::shared_ptr<wrench::BareMetalComputeService> baremetal_service = nullptr;

    void do_IntegrationFailureTest_test(std::map<std::string, bool> args);


protected:
    std::string createRoute(const std::string &src, const std::string &dst, const std::vector<std::string> &links) {
        std::string to_return = "<route src=\"" + src + "\" ";
        to_return += "dst=\"" + dst + "\">\n";
        for (auto const &l: links) {
            to_return += "<link_ctn id=\"" + l + "\"/>\n";
        }
        to_return += "</route>\n";
        return to_return;
    }

    ~ComprehensiveIntegrationHostFailuresTest() override {
        workflow->clear();
        wrench::Simulation::removeAllFiles();
    }

    ComprehensiveIntegrationHostFailuresTest() {
        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();


        // Create a platform file
        std::string xml = "<?xml version='1.0'?>\n"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">\n"
                          "<platform version=\"4.1\">\n"
                          "   <zone id=\"AS0\" routing=\"Full\">\n";

        std::vector<std::string> hostnames = {"WMSHost",
                                              "StorageHost1", "StorageHost2",
                                              "CloudHead", "CloudHost1", "CloudHost2", "CloudHost3",
                                              "BareMetalHead", "BareMetalHost1", "BareMetalHost2"};

        for (auto const &h: hostnames) {
            xml += "<host id=\"" + h + "\"  speed=\"1f\" core=\"4\" > \n"
                                       "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">\n"
                                       "             <prop id=\"size\" value=\"10000000000000B\"/>\n"
                                       "             <prop id=\"mount\" value=\"/\"/>\n"
                                       "          </disk>\n"
                                       "          <disk id=\"scratch_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">\n"
                                       "             <prop id=\"size\" value=\"100B\"/>\n"
                                       "             <prop id=\"mount\" value=\"/scratch\"/>\n"
                                       "          </disk>\n"
                                       "          <prop id=\"ram\" value=\"100B\" /> \n"
                                       "</host>\n";
        }

        xml += "<link id=\"link\" bandwidth=\"1kBps\" latency=\"0\"/>\n";
        xml += "<link id=\"linkcloud\" bandwidth=\"1kBps\" latency=\"0\"/>\n";
        xml += "<link id=\"linkcloudinternal\" bandwidth=\"1kBps\" latency=\"0\"/>\n";
        xml += "<link id=\"linkbaremetal\" bandwidth=\"1kBps\" latency=\"0\"/>\n";
        xml += "<link id=\"linkbaremetalinternal\" bandwidth=\"1kBps\" latency=\"0\"/>\n";
        xml += "<link id=\"linkstorage1\" bandwidth=\"1kBps\" latency=\"0\"/>\n";
        xml += "<link id=\"linkstorage2\" bandwidth=\"1kBps\" latency=\"0\"/>\n";

        xml += createRoute("WMSHost", "CloudHead", {"link", "linkcloud"});
        xml += createRoute("WMSHost", "BareMetalHead", {"link", "linkbaremetal"});
        xml += createRoute("WMSHost", "StorageHost1", {"link", "linkstorage1"});
        xml += createRoute("WMSHost", "StorageHost2", {"link", "linkstorage2"});

        xml += createRoute("CloudHead", "CloudHost1", {"link", "linkcloudinternal"});
        xml += createRoute("CloudHead", "CloudHost2", {"link", "linkcloudinternal"});
        xml += createRoute("CloudHead", "CloudHost3", {"link", "linkcloudinternal"});

        xml += createRoute("BareMetalHead", "BareMetalHost1", {"link", "linkbaremetalinternal"});
        xml += createRoute("BareMetalHead", "BareMetalHost2", {"link", "linkbaremetalinternal"});

        xml += createRoute("StorageHost1", "CloudHost1", {"linkstorage1", "linkcloud", "linkcloudinternal"});
        xml += createRoute("StorageHost1", "CloudHost2", {"linkstorage1", "linkcloud", "linkcloudinternal"});
        xml += createRoute("StorageHost1", "CloudHost3", {"linkstorage1", "linkcloud", "linkcloudinternal"});
        xml += createRoute("StorageHost2", "CloudHost1", {"linkstorage2", "linkcloud", "linkcloudinternal"});
        xml += createRoute("StorageHost2", "CloudHost2", {"linkstorage2", "linkcloud", "linkcloudinternal"});
        xml += createRoute("StorageHost2", "CloudHost3", {"linkstorage2", "linkcloud", "linkcloudinternal"});

        xml += createRoute("StorageHost1", "BareMetalHost1", {"linkstorage1", "linkbaremetal", "linkbaremetalinternal"});
        xml += createRoute("StorageHost1", "BareMetalHost2", {"linkstorage1", "linkbaremetal", "linkbaremetalinternal"});
        xml += createRoute("StorageHost2", "BareMetalHost1", {"linkstorage2", "linkbaremetal", "linkbaremetalinternal"});
        xml += createRoute("StorageHost2", "BareMetalHost2", {"linkstorage2", "linkbaremetal", "linkbaremetalinternal"});

        xml += "   </zone> \n"
               "</platform>\n";

#if 0

        "       <host id=\"StorageHost1\"   speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"StorageHost2\"   speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"CloudHead\"      speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"CloudHost1\"     speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"CloudHost2\"     speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"CloudHost3\"     speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"BareMetalHead\"  speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"BareMetalHost1\" speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"BareMetalHost2\" speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"WMSHost\"        speed=\"1f\" core=\"1\"/> "
                          "       <link id=\"link1\" bandwidth=\"1kBps\" latency=\"0\"/>"
                          "       <route src=\"FailedHost\" dst=\"StableHost\">"
                          "           <link_ctn id=\"link1\"/>"
                          "       </route>"
                          "   </zone> "
                          "</platform>";

#endif
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};


/**********************************************************************/
/**          INTEGRATION TEST                                        **/
/**********************************************************************/

class IntegrationFailureTestTestWMS : public wrench::ExecutionController {

public:
    IntegrationFailureTestTestWMS(ComprehensiveIntegrationHostFailuresTest *test,
                                  std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    std::set<std::string> vms;
    std::map<std::shared_ptr<wrench::BareMetalComputeService>, bool> vm_used;
    ComprehensiveIntegrationHostFailuresTest *test;
    std::shared_ptr<wrench::JobManager> job_manager;
    unsigned long num_jobs_on_baremetal_cs = 0;
    unsigned long max_num_jobs_on_baremetal_cs = 4;

    void createMonkey(const std::string &victimhost, double alpha = 1.0) {
        static unsigned long seed = 666;
        seed = seed * 17 + 37;
        auto switcher = std::make_shared<wrench::ResourceRandomRepeatSwitcher>(
                "WMSHost", seed,
                CHAOS_MONKEY_MIN_SLEEP_BEFORE_OFF, CHAOS_MONKEY_MAX_SLEEP_BEFORE_OFF,
                CHAOS_MONKEY_MIN_SLEEP_BEFORE_ON, CHAOS_MONKEY_MAX_SLEEP_BEFORE_ON,
                victimhost, wrench::ResourceRandomRepeatSwitcher::ResourceType::HOST);
        switcher->setSimulation(this->getSimulation());
        switcher->start(switcher, true, false);// Daemonized, no auto-restart
    }

    int main() override {

        this->job_manager = createJobManager();

        if (this->test->faulty_map.find("cloud") != this->test->faulty_map.end()) {
            // Create my sef of VMs
            try {
                for (int i = 0; i < 2; i++) {// TODO: MAKE IT 6
                    auto vm_name = this->test->cloud_service->createVM(2, 45);
                    this->test->cloud_service->startVM(vm_name);
                    this->vms.insert(vm_name);
                }
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Should be able to create VMs!!");
            }
        }

        // Creating Chaos Monkeys
        for (auto const &faulty: this->test->faulty_map) {
            if (not faulty.second) {
                continue;
            }
            if (faulty.first == "cloud") {
                createMonkey("CloudHost1");
                //                createMonkey("CloudHost2");
                //                createMonkey("CloudHost3");

            } else if (faulty.first == "baremetal") {
                createMonkey("BareMetalHost1");
                createMonkey("BareMetalHost2");
            } else if (faulty.first == "storage1") {
                createMonkey("StorageHost1");
            } else if (faulty.first == "storage2") {
                createMonkey("StorageHost2");
            }
        }


        while (not this->test->workflow->isDone()) {

            if (wrench::Simulation::getCurrentSimulatedDate() > 17508.33982809935332625173) {
                break;
            }

            // Try to restart down VMs
            for (auto const &vm: this->vms) {
                if (this->test->cloud_service->isVMDown(vm)) {
                    try {
                        this->test->cloud_service->startVM(vm);
                    } catch (wrench::ExecutionException &e) {
                        if (std::dynamic_pointer_cast<wrench::NotEnoughResources>(e.getCause())) {
                            //  WRENCH_INFO("Cannot start VM");
                        } else if (std::dynamic_pointer_cast<wrench::HostError>(e.getCause())) {
                            //  WRENCH_INFO("Cannot start VM because Host is down");
                        } else {
                            throw;
                        }
                    }
                }
            }

            while (scheduleAReadyTask()) {
            }

            if (not this->waitForAndProcessNextEvent(50.00)) {
            }
        }

        return 0;
    }

    bool scheduleAReadyTask() {


        // Find a ready task1
        std::shared_ptr<wrench::WorkflowTask> task = nullptr;
        for (auto const &t: this->test->workflow->getTasks()) {
            if (t->getState() == wrench::WorkflowTask::READY) {
                task = t;
                break;
            }
        }
        if (not task) {
            return false;// no ready task right now
        }

        // Pick a storage service
        std::shared_ptr<wrench::StorageService> target_storage_service;

        if (this->test->storage_service1 and this->test->storage_service1->isUp()) {
            target_storage_service = this->test->storage_service1;
        } else if (this->test->storage_service2 and this->test->storage_service2->isUp()) {
            target_storage_service = this->test->storage_service2;
        } else {
            return false;
        }

        // Pick a compute resource (trying the cloud first)
        std::shared_ptr<wrench::ComputeService> target_cs = nullptr;
        for (auto &vm: this->vms) {
            auto vm_cs = this->test->cloud_service->getVMComputeService(vm);
            if (vm_cs->isUp() and (not this->vm_used[vm_cs])) {
                target_cs = vm_cs;
                this->vm_used[vm_cs] = true;
                break;
            }
        }

        if ((target_cs == nullptr) and (this->test->baremetal_service != nullptr)) {
            if (num_jobs_on_baremetal_cs < max_num_jobs_on_baremetal_cs) {
                target_cs = this->test->baremetal_service;
                num_jobs_on_baremetal_cs++;
            }
        }

        if (target_cs == nullptr) {
            return false;
        }

        // Create/submit a standard job
        auto job = this->job_manager->createStandardJob(task, {
                                                                      {task->getInputFiles().at(0), wrench::FileLocation::LOCATION(target_storage_service, task->getInputFiles().at(0))},
                                                                      {task->getOutputFiles().at(0), wrench::FileLocation::LOCATION(target_storage_service, task->getOutputFiles().at(0))},
                                                              });
        this->job_manager->submitJob(job, target_cs);


        //        WRENCH_INFO("Submitted task1 '%s' to '%s' with files to read from '%s",
        //                    task1->getID().c_str(),
        //                    target_cs->getName().c_str(),
        //                    target_storage_service->getName().c_str());
        return true;
    }

    void processEventStandardJobCompletion(const std::shared_ptr<wrench::StandardJobCompletedEvent> &event) override {
        static int count = 0;
        auto task = *(event->standard_job->getTasks().begin());
        count++;
        WRENCH_INFO("Task '%s' has completed", task->getID().c_str());
        if (event->compute_service == this->test->baremetal_service) {
            num_jobs_on_baremetal_cs--;
        } else {
            for (auto const &u: this->vm_used) {
                if (u.first == event->compute_service) {
                    this->vm_used[u.first] = false;
                }
            }
        }
    }

    void processEventStandardJobFailure(const std::shared_ptr<wrench::StandardJobFailedEvent> &event) override {
        auto task = *(event->standard_job->getTasks().begin());

        WRENCH_INFO("Task '%s' has failed: %s", task->getID().c_str(), event->failure_cause->toString().c_str());
        if (event->compute_service == this->test->baremetal_service) {
            num_jobs_on_baremetal_cs--;
        } else {
            for (auto const &u: this->vm_used) {
                if (u.first == event->compute_service) {
                    this->vm_used[u.first] = false;
                }
            }
        }
    }
};

TEST_F(ComprehensiveIntegrationHostFailuresTest, OneNonFaultyStorageOneFaultyBareMetal) {
    std::map<std::string, bool> args;
    args["storage1"] = false;
    args["baremetal"] = true;
    DO_TEST_WITH_FORK_ONE_ARG(do_IntegrationFailureTest_test, args);
}

TEST_F(ComprehensiveIntegrationHostFailuresTest, OneFaultyStorageOneNonFaultyBareMetal) {
    std::map<std::string, bool> args;
    args["storage1"] = true;
    args["baremetal"] = false;
    DO_TEST_WITH_FORK_ONE_ARG(do_IntegrationFailureTest_test, args);
}

TEST_F(ComprehensiveIntegrationHostFailuresTest, OneFaultyStorageOneFaultyBareMetal) {
    std::map<std::string, bool> args;
    args["storage1"] = true;
    args["baremetal"] = true;
    DO_TEST_WITH_FORK_ONE_ARG(do_IntegrationFailureTest_test, args);
}

TEST_F(ComprehensiveIntegrationHostFailuresTest, TwoFaultyStorageOneFaultyBareMetal) {
    std::map<std::string, bool> args;
    args["storage1"] = true;
    args["storage2"] = true;
    args["baremetal"] = true;
    DO_TEST_WITH_FORK_ONE_ARG(do_IntegrationFailureTest_test, args);
}

TEST_F(ComprehensiveIntegrationHostFailuresTest, OneNonFaultyStorageOneFaultyCloud) {
    std::map<std::string, bool> args;
    args["storage1"] = false;
    args["cloud"] = true;
    DO_TEST_WITH_FORK_ONE_ARG(do_IntegrationFailureTest_test, args);
}

TEST_F(ComprehensiveIntegrationHostFailuresTest, OneFaultyStorageOneFaultyCloud) {
    std::map<std::string, bool> args;
    args["storage1"] = true;
    args["cloud"] = true;
    DO_TEST_WITH_FORK_ONE_ARG(do_IntegrationFailureTest_test, args);
}

TEST_F(ComprehensiveIntegrationHostFailuresTest, TwoFaultyStorageOneFaultyCloud) {
    std::map<std::string, bool> args;
    args["storage1"] = true;
    args["storage2"] = true;
    args["cloud"] = true;
    DO_TEST_WITH_FORK_ONE_ARG(do_IntegrationFailureTest_test, args);
}

TEST_F(ComprehensiveIntegrationHostFailuresTest, WholeEnchilada) {
    std::map<std::string, bool> args;
    args["storage1"] = true;
    args["storage2"] = true;
    args["baremetal"] = true;
    args["cloud"] = true;
    DO_TEST_WITH_FORK_ONE_ARG(do_IntegrationFailureTest_test, args);
}

void ComprehensiveIntegrationHostFailuresTest::do_IntegrationFailureTest_test(std::map<std::string, bool> args) {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 3;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");
    argv[2] = strdup("--cfg=contexts/stack-size:100");
    //    argv[3] = strdup("--wrench-full-log");
    //    argv[4] = strdup("--wrench-default-control-message-size=0");
    //        argv[4] = strdup("--wrench-no-color");

    this->faulty_map = args;

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create Storage Services
    if (args.find("storage1") != args.end()) {
        this->storage_service1 = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
                "StorageHost1", {"/"},
                {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(1000000)}}, {}));
    }
    if (args.find("storage2") != args.end()) {
        this->storage_service2 = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
                "StorageHost2", {"/"},
                {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(1000000)}}, {}));
    }

    ASSERT_TRUE((this->storage_service1 != nullptr) or (this->storage_service2 != nullptr));

    // Create BareMetal Service
    if (args.find("baremetal") != args.end()) {
        this->baremetal_service = std::dynamic_pointer_cast<wrench::BareMetalComputeService>(simulation->add(
                new wrench::BareMetalComputeService(
                        "BareMetalHead",
                        (std::map<std::string, std::tuple<unsigned long, sg_size_t>>){
                                std::make_pair("BareMetalHost1", std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                 wrench::ComputeService::ALL_RAM)),
                                std::make_pair("BareMetalHost2", std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                 wrench::ComputeService::ALL_RAM)),
                        },
                        "/scratch",
                        {{wrench::BareMetalComputeServiceProperty::FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH, "false"}}, {})));
    }

    // Create Cloud Service
    if (args.find("cloud") != args.end()) {
        std::string cloudhead = "CloudHead";
        std::vector<std::string> cloudhosts;
        cloudhosts.emplace_back("CloudHost1");
        //        cloudhosts.push_back("CloudHost2");
        //        cloudhosts.push_back("CloudHost3");
        this->cloud_service = std::dynamic_pointer_cast<wrench::CloudComputeService>(
                simulation->add(new wrench::CloudComputeService(
                        cloudhead,
                        cloudhosts,
                        {"/scratch"},
                        {}, {})));
    }

    // Create a FileRegistryService
    simulation->add(new wrench::FileRegistryService("WMSHost"));

    // Create workflow tasks and stage input file
    srand(666);
    for (int i = 0; i < NUM_TASKS; i++) {
        //        auto task1 = workflow->addTask("task_" + std::to_string(i), 1 + rand() % MAX_TASK_DURATION_WITH_ON_CORE, 1, 3, 1.0, 0);
        auto task = workflow->addTask("task_" + std::to_string(i), MAX_TASK_DURATION_WITH_ON_CORE, 1, 3, 40);
        auto input_file = wrench::Simulation::addFile(task->getID() + ".input", 1 + rand() % 100);
        auto output_file = wrench::Simulation::addFile(task->getID() + ".output", 1 + rand() % 100);
        task->addInputFile(input_file);
        task->addOutputFile(output_file);
        if (this->storage_service1) {
            this->storage_service1->createFile(input_file);
        }
        if (this->storage_service2) {
            this->storage_service2->createFile(input_file);
        }
    }

    // Create a WMS
    std::string wms_host = "WMSHost";
    auto wms = simulation->add(
            new IntegrationFailureTestTestWMS(
                    this,
                    wms_host));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
