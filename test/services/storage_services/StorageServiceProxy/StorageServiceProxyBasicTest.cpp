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


WRENCH_LOG_CATEGORY(storage_service_proxy_basic_functional_test, "Log category for StorageServiceProxyBasicTest");

class StorageServiceProxyBasicTest : public ::testing::Test {

public:
   void do_BasicFunctionality_test(bool arg);

   std::shared_ptr<wrench::SimpleStorageService> remote;
   std::shared_ptr<wrench::SimpleStorageService> target;
   std::shared_ptr<wrench::StorageServiceProxy> proxy;

   protected:
    StorageServiceProxyBasicTest() {


       // Create a one-host platform file
       std::string xml = "<?xml version='1.0'?>"
                         "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                         "<platform version=\"4.1\"> "
                         "   <zone id=\"AS0\" routing=\"Full\"> "
                         "       <host id=\"Client\" speed=\"1f\">"
                         "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                         "             <prop id=\"size\" value=\"100GB\"/>"
                         "             <prop id=\"mount\" value=\"/disk100/\"/>"
                         "          </disk>"
                         "       </host>"
                         "       <host id=\"Proxy\" speed=\"1f\">"
                         "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                         "             <prop id=\"size\" value=\"100GB\"/>"
                         "             <prop id=\"mount\" value=\"/disk100/\"/>"
                         "          </disk>"
                         "       </host>"
                         "       <host id=\"Remote\" speed=\"1f\">"
                         "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                         "             <prop id=\"size\" value=\"100GB\"/>"
                         "             <prop id=\"mount\" value=\"/disk100/\"/>"
                         "          </disk>"
                         "       </host>"
                         "       <host id=\"Target\" speed=\"1f\">"
                         "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                         "             <prop id=\"size\" value=\"100GB\"/>"
                         "             <prop id=\"mount\" value=\"/disk100/\"/>"
                         "          </disk>"
                         "       </host>"
                         "       <link id=\"link12\" bandwidth=\"5GBps\" latency=\"10us\"/>"
                         "       <link id=\"link13\" bandwidth=\"100MBps\" latency=\"100us\"/>"
                         "       <link id=\"link23\" bandwidth=\"100MBps\" latency=\"100us\"/>"
                         "       <route src=\"Client\" dst=\"Proxy\"> <link_ctn id=\"link12\"/> </route>"
                         "       <route src=\"Proxy\" dst=\"Remote\"> <link_ctn id=\"link13\"/> </route>"
                         "       <route src=\"Proxy\" dst=\"Target\"> <link_ctn id=\"link23\"/> </route>"
                         "   </zone> "
                         "</platform>";
       FILE *platform_file = fopen(platform_file_path.c_str(), "w");
       fprintf(platform_file, "%s", xml.c_str());
       fclose(platform_file);
   }

   std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};


/**********************************************************************/
/**  BASIC FUNCTIONALITY SIMULATION TEST                             **/
/**********************************************************************/

class StorageServiceProxyBasicTestExecutionController : public wrench::ExecutionController {

public:
    StorageServiceProxyBasicTestExecutionController(StorageServiceProxyBasicTest *test,
                                                          std::string hostname,bool testWithDefault) : wrench::ExecutionController(hostname, "test"), test(test),testWithDefault(testWithDefault) {
   }

private:
    StorageServiceProxyBasicTest *test;
    bool testWithDefault;
   int main() override {
       using namespace wrench;
       WRENCH_INFO("Adding a file files to the simulation");
       auto file1 = wrench::Simulation::addFile("file1", 10000);
       auto file2 = wrench::Simulation::addFile("file2", 10000);
       auto file3 = wrench::Simulation::addFile("file3", 10000);

       try {
           this->test->proxy->createFile(file1);
           throw std::runtime_error("Should not be able to create a file on a proxy directly");
       } catch (std::runtime_error &ignore) {}

       // Create a copy file1 on remote
       this->test->remote->createFile(file1);
       // Create a copy file1 on target
       this->test->target->createFile(file2);

       // Create a copy file3 on remote and in cache
       this->test->remote->createFile(file3);
       this->test->proxy->getCache()->createFile(file3);

       // locate file1 via proxy
//       if(testWithDefault) ((shared_ptr<StorageService>)this->test->proxy)->lookupFile(file1);
       if(testWithDefault) this->test->proxy->lookupFile(file1);

       // Try to read file2 from remote although it is on target
       try {
           this->test->proxy->readFile(file2);
           throw std::runtime_error("Non extant files should throw exceptions when not found");
       } catch (wrench::ExecutionException &ignore) {
           if (not std::dynamic_pointer_cast<wrench::FileNotFound>(ignore.getCause())) {
               throw std::runtime_error("Should have received a FileNotFound execution when reading file2 from supervisor");
           }
       }
       // locate file2 via proxy and target
       this->test->proxy->lookupFile(this->test->remote,file1);


       //TODO insert timing checks

       // Read file1 via proxy
       if(testWithDefault) this->test->proxy->readFile(file1);
       //read file again to check cache
       this->test->proxy->readFile(file1);

       // Try to read file2 from remote although it is on target
       try {
           this->test->proxy->readFile(file2);
           throw std::runtime_error("Non extant files should throw exceptions when not found");
       } catch (wrench::ExecutionException &ignore) {
           if (not std::dynamic_pointer_cast<wrench::FileNotFound>(ignore.getCause())) {
               throw std::runtime_error("Should have received a FileNotFound execution when reading file2 from supervisor");
           }
       }
       // read file2 via proxy and target
       this->test->proxy->readFile(this->test->remote,file1);

       //write file tests
       //if(testWithDefault)
       //delete file tests
       //if(testWithDefault)
       return 0;

   }
};

TEST_F(StorageServiceProxyBasicTest, BasicFunctionalityNoDefaultLocation) {
   DO_TEST_WITH_FORK_ONE_ARG(do_BasicFunctionality_test, false);
}
TEST_F(StorageServiceProxyBasicTest, BasicFunctionalityDefaultLocation) {
   DO_TEST_WITH_FORK_ONE_ARG(do_BasicFunctionality_test, true);
}

void StorageServiceProxyBasicTest::do_BasicFunctionality_test(bool arg) {

   // Create and initialize a simulation
   auto simulation = wrench::Simulation::createSimulation();
   int argc = 1;
   char **argv = (char **) calloc(argc, sizeof(char *));
   argv[0] = strdup("unit_test");
   //    argv[1] = strdup("--wrench-full-log");

   simulation->init(&argc, argv);

   // Setting up the platform
   simulation->instantiatePlatform(platform_file_path);

   // Create a XRootD Manager object


   this->remote = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
           "Remote", {"/disk100"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "10MB"}}, {}));
   auto cache = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
           "Proxy", {"/disk100"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "10MB"}}, {}));
   this->target = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
           "Target", {"/disk100"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "10MB"}}, {}));

   if(arg){
       this->proxy = simulation->add(
               wrench::StorageServiceProxy::createRedirectProxy(
                       "Proxy", cache
               )
       );
   }else{
       this->proxy = simulation->add(
               wrench::StorageServiceProxy::createRedirectProxy(
                       "Proxy", cache,target
                       )
       );

   }
   // Create an execution controller
   auto controller = simulation->add(new StorageServiceProxyBasicTestExecutionController(this, "Client",arg));

   // Running a "run a single task1" simulation
   ASSERT_NO_THROW(simulation->launch());

   for (int i = 0; i < argc; i++)
       free(argv[i]);
   free(argv);
}
