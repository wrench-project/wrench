/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <wrench-dev.h>
#include <wrench/services/helpers/ServiceTerminationDetectorMessage.h>

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"
#include "failure_test_util/HostSwitcher.h"
#include "wrench/services/helpers/ServiceTerminationDetector.h"
#include "./failure_test_util/SleeperVictim.h"
#include "./failure_test_util/ComputerVictim.h"
#include "failure_test_util/HostRandomRepeatSwitcher.h"


XBT_LOG_NEW_DEFAULT_CATEGORY(comprehensive_failure_integration_test, "Log category for ComprehesiveIntegrationFailureTest");

#define NUM_TASKS 100
#define MAX_TASK_DURATION_WITH_ON_CORE 3600
#define CHAOS_MONKEY_MIN_SLEEP_BEFORE_OFF 100
#define CHAOS_MONKEY_MAX_SLEEP_BEFORE_OFF 4000
#define CHAOS_MONKEY_MIN_SLEEP_BEFORE_ON 100
#define CHAOS_MONKEY_MAX_SLEEP_BEFORE_ON 1000

class IntegrationSimulatedFailuresTest : public ::testing::Test {

public:
    wrench::Workflow *workflow;
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr;
    std::map<std::string, bool> faulty_map;

    wrench::StorageService *storage_service1 = nullptr;
    wrench::StorageService *storage_service2 = nullptr;
    wrench::CloudService *cloud_service = nullptr;
    wrench::BareMetalComputeService *baremetal_service = nullptr;

    void do_IntegrationFailureTestTest_test(std::map<std::string, bool> args);


protected:

    std::string createRoute(std::string src, std::string dst, std::vector<std::string> links) {
        std::string to_return = "<route src=\"" +  src + "\" ";
        to_return += "dst=\"" + dst + "\">\n";
        for (auto const &l : links) {
            to_return += "<link_ctn id=\"" + l + "\"/>\n";
        }
        to_return += "</route>\n";
        return to_return;
    }

    IntegrationSimulatedFailuresTest() {
        // Create the simplest workflow
        workflow_unique_ptr = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
        workflow = workflow_unique_ptr.get();

        // Create a platform file
        std::string xml = "<?xml version='1.0'?>\n"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">\n"
                          "<platform version=\"4.1\">\n"
                          "   <zone id=\"AS0\" routing=\"Full\">\n";

        std::vector<std::string> hostnames = {"WMSHost",
                                              "StorageHost1", "StorageHost2",
                                              "CloudHead", "CloudHost1", "CloudHost2", "CloudHost3",
                                              "BareMetalHead", "BareMetalHost1", "BareMetalHost2"};

        for (auto const &h : hostnames) {
            xml += "<host id=\"" + h + "\"  speed=\"1f\" core=\"4\"/>\n";
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

class IntegrationFailureTestTestWMS : public wrench::WMS {

public:
    IntegrationFailureTestTestWMS(IntegrationSimulatedFailuresTest *test,
                                  std::string &hostname,
                                  std::set<wrench::ComputeService *> compute_services,
                                  std::set<wrench::StorageService *> storage_services
    ) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    std::map<std::string, std::shared_ptr<wrench::BareMetalComputeService>> vms;
    std::map<std::shared_ptr<wrench::BareMetalComputeService>, bool> vm_used;
    IntegrationSimulatedFailuresTest *test;
    std::shared_ptr<wrench::JobManager> job_manager;
    unsigned long num_jobs_on_baremetal_cs = 0;
    unsigned long max_num_jobs_on_baremetal_cs = 4;

    void createMonkey(std::string victimhost, double alpha = 1.0) {
        static unsigned long seed = 666;
        seed = seed * 17 + 37;
        auto switcher = std::shared_ptr<wrench::HostRandomRepeatSwitcher>(
                new wrench::HostRandomRepeatSwitcher("WMSHost", seed,
                                                     CHAOS_MONKEY_MIN_SLEEP_BEFORE_OFF, CHAOS_MONKEY_MAX_SLEEP_BEFORE_OFF,
                                                     CHAOS_MONKEY_MIN_SLEEP_BEFORE_ON, CHAOS_MONKEY_MAX_SLEEP_BEFORE_ON,
                                                     victimhost));
        switcher->simulation = this->simulation;
        switcher->start(switcher, true, false); // Daemonized, no auto-restart
    }

    int main() override {

        this->job_manager = createJobManager();

        if (this->test->faulty_map.find("cloud") != this->test->faulty_map.end()) {
            // Create my sef of VMs
            try {
                for (int i = 0; i < 6; i++) {
                    auto vm_name = this->test->cloud_service->createVM(2, 1000);
                    auto vm_cs = this->test->cloud_service->startVM(vm_name);
                    this->vms[vm_name] = vm_cs;
                    this->vm_used[vm_cs] = false;
                }
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error("Should be able to create VMs!!");
            }
        }

        // Creating Chaos Monkeys
        for (auto const &faulty : this->test->faulty_map) {
            if (not faulty.second) {
                continue;
            }
            if (faulty.first == "cloud") {
                createMonkey("CloudHost1");
                createMonkey("CloudHost2");
                createMonkey("CloudHost3");

            } else if (faulty.first == "baremetal") {
                createMonkey("BareMetalHost1");
                createMonkey("BareMetalHost2");
            } else if (faulty.first == "storage1") {
                createMonkey("StorageHost1");
            } else if (faulty.first == "storage2") {
                createMonkey("StorageHost2");
            }
        }


        while (not this->getWorkflow()->isDone()) {

            // Try to restart down VMs
            for (auto const &vm : this->vms) {
                if (this->test->cloud_service->isVMDown(vm.first)) {
                    try {
                        this->test->cloud_service->startVM(vm.first);
                    } catch (wrench::WorkflowExecutionException &e) {
                        if (e.getCause()->getCauseType() == wrench::FailureCause::NOT_ENOUGH_RESOURCES) {
                            // oh well
                        } else {
                            throw;
                        }
                    }
                }
            }

            while (scheduleAReadyTask()) {}

            if (not this->waitForAndProcessNextEvent(50.00)) {
//                WRENCH_INFO("TIMEOUT");
            }
        }
        WRENCH_INFO("WORKFLOW DONE!");

        return 0;
    }

    bool scheduleAReadyTask() {

        // Find a ready task
        wrench::WorkflowTask *task = nullptr;
        for (auto const &t : this->getWorkflow()->getTasks()) {
            if (t->getState() == wrench::WorkflowTask::READY) {
                task = t;
                break;
            }
        }
        if (not task) return false; // no ready task right now

        // Pick a storage service
        wrench::StorageService *target_storage_service;

        if (this->test->storage_service1 and this->test->storage_service1->isUp()) {
            target_storage_service = this->test->storage_service1;
        } else if (this->test->storage_service2 and this->test->storage_service2->isUp()) {
            target_storage_service = this->test->storage_service2;
        } else {
            return false;
        }

        // Pick a compute resource (trying the cloud first)
        wrench::ComputeService *target_cs = nullptr;
        for (auto &vm : this->vms) {
            auto vm_cs = vm.second;
            if (vm_cs->isUp() and (not this->vm_used[vm_cs])) {
                target_cs = vm_cs.get();
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
        auto job = this->job_manager->createStandardJob(task, {{*(task->getInputFiles().begin()), target_storage_service}});
        this->job_manager->submitJob(job, target_cs);

//        WRENCH_INFO("Submitted task '%s' to '%s' with files to read from '%s",
//                    task->getID().c_str(),
//                    target_cs->getName().c_str(),
//                    target_storage_service->getName().c_str());
        return true;
    }

    void processEventStandardJobCompletion(std::unique_ptr<wrench::StandardJobCompletedEvent> event) override {
        auto task = *(event->standard_job->getTasks().begin());
//        WRENCH_INFO("Task '%s' has completed", task->getID().c_str());
        if (event->compute_service == this->test->baremetal_service) {
            num_jobs_on_baremetal_cs--;
        } else {
            for (auto const &u : this->vm_used) {
                if (u.first.get() == event->compute_service) {
                    this->vm_used[u.first] = false;
                }
            }
        }
    }

    void processEventStandardJobFailure(std::unique_ptr<wrench::StandardJobFailedEvent> event) override {
        auto task = *(event->standard_job->getTasks().begin());
//        WRENCH_INFO("Task '%s' has failed: %s", task->getID().c_str(), event->failure_cause->toString().c_str());
        if (event->compute_service == this->test->baremetal_service) {
            num_jobs_on_baremetal_cs--;
        } else {
            for (auto const &u : this->vm_used) {
                if (u.first.get() == event->compute_service) {
                    this->vm_used[u.first] = false;
                }
            }
        }
    }


};

TEST_F(IntegrationSimulatedFailuresTest, OneNonFaultyStorageOneFaultyBareMetal) {
    std::map<std::string, bool> args;
    args["storage1"] = false;
    args["baremetal"] = true;
    DO_TEST_WITH_FORK_ONE_ARG(do_IntegrationFailureTestTest_test, args);
}

TEST_F(IntegrationSimulatedFailuresTest, OneFaultyStorageOneFaultyBareMetal) {
    std::map<std::string, bool> args;
    args["storage1"] = true;
    args["baremetal"] = true;
    DO_TEST_WITH_FORK_ONE_ARG(do_IntegrationFailureTestTest_test, args);
}

TEST_F(IntegrationSimulatedFailuresTest, TwoFaultyStorageOneFaultyBareMetal) {
    std::map<std::string, bool> args;
    args["storage1"] = true;
    args["storage2"] = true;
    args["baremetal"] = true;
    DO_TEST_WITH_FORK_ONE_ARG(do_IntegrationFailureTestTest_test, args);
}

TEST_F(IntegrationSimulatedFailuresTest, OneNonFaultyStorageOneFaultyCloud) {
    std::map<std::string, bool> args;
    args["storage1"] = false;
    args["cloud"] = true;
    DO_TEST_WITH_FORK_ONE_ARG(do_IntegrationFailureTestTest_test, args);
}

TEST_F(IntegrationSimulatedFailuresTest, OneFaultyStorageOneFaultyCloud) {
    std::map<std::string, bool> args;
    args["storage1"] = true;
    args["cloud"] = true;
    DO_TEST_WITH_FORK_ONE_ARG(do_IntegrationFailureTestTest_test, args);
}

TEST_F(IntegrationSimulatedFailuresTest, TwoFaultyStorageOneFaultyCloud) {
    std::map<std::string, bool> args;
    args["storage1"] = true;
    args["storage2"] = true;
    args["cloud"] = true;
    DO_TEST_WITH_FORK_ONE_ARG(do_IntegrationFailureTestTest_test, args);
}

void IntegrationSimulatedFailuresTest::do_IntegrationFailureTestTest_test(std::map<std::string, bool> args) {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("failure_test");

    this->faulty_map = args;

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create Storage Services
    if (args.find("storage1") != args.end()) {
        this->storage_service1 = simulation->add(new wrench::SimpleStorageService("StorageHost1", 10000000000000.0));
    }
    if (args.find("storage2") != args.end()) {
        this->storage_service2 = simulation->add(new wrench::SimpleStorageService("StorageHost2", 10000000000000.0));
    }

    ASSERT_TRUE((this->storage_service1 != nullptr) or (this->storage_service2 != nullptr));

    // Create BareMetal Service
    if (args.find("baremetal") != args.end()) {
        this->baremetal_service = (wrench::BareMetalComputeService *) simulation->add(
                new wrench::BareMetalComputeService(
                        "BareMetalHead",
                        (std::map<std::string, std::tuple<unsigned long, double>>) {
                                std::make_pair("BareMetalHost1", std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                 wrench::ComputeService::ALL_RAM)),
                                std::make_pair("BareMetalHost2", std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                 wrench::ComputeService::ALL_RAM)),
                        },
                        100.0,
                        {}, {}));
    }

    // Create Cloud Service
    if (args.find("cloud") != args.end()) {
        std::string cloudhead = "CloudHead";
        std::vector<std::string> cloudhosts;
        cloudhosts.push_back("CloudHost1");
        cloudhosts.push_back("CloudHost2");
        cloudhosts.push_back("CloudHost3");
        this->cloud_service = (wrench::CloudService *) simulation->add(new wrench::CloudService(
                cloudhead,
                cloudhosts,
                1000000.0,
                {}, {}));
    }

    // Create a FileRegistryService
    simulation->add(new wrench::FileRegistryService("WMSHost"));

    // Create workflow tasks and stage input file
    for (int i=0; i < NUM_TASKS; i++) {
        auto task = workflow->addTask("task_" + std::to_string(i), 1 + rand() % MAX_TASK_DURATION_WITH_ON_CORE, 1, 3, 1.0, 0);
        auto input_file = workflow->addFile(task->getID() + ".input", 1 + rand() % 100);
        auto output_file = workflow->addFile(task->getID() + ".output", 1 + rand() % 100);
        task->addInputFile(input_file);
        task->addOutputFile(output_file);
        if (this->storage_service1) {
            simulation->stageFiles({{input_file->getID(), input_file}}, this->storage_service1);
        }
        if (this->storage_service2) {
            simulation->stageFiles({{input_file->getID(), input_file}}, this->storage_service2);
        }
    }

    // Create a WMS
    std::string wms_host = "WMSHost";
    auto wms = simulation->add(
            new IntegrationFailureTestTestWMS(
                    this,
                    wms_host,
                    {this->baremetal_service, this->cloud_service},
                    {this->storage_service1, this->storage_service2}));

    wms->addWorkflow(workflow);


    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}




