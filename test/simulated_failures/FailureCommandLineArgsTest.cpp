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

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"

class FailureCommandLineArgsTest : public ::testing::Test {

public:
    void do_MissingFailureCommandLineArgsTest_test();

protected:
    FailureCommandLineArgsTest() {
        // Create a platform file
        std::string xml = R"(<?xml version='1.0'?>
                          <!DOCTYPE platform SYSTEM "https://simgrid.org/simgrid.dtd">
                          <platform version="4.1"> 
                             <zone id="AS0" routing="Full"> 
                                 <host id="DualCoreHost" speed="1f" core="2" > 
                                    <disk id="large_disk1" read_bw="100MBps" write_bw="100MBps">
                                       <prop id="size" value="100B"/>
                                       <prop id="mount" value="/disk1"/>
                                    </disk>
                                    <disk id="large_disk2" read_bw="100MBps" write_bw="100MBps">
                                       <prop id="size" value="100B"/>
                                       <prop id="mount" value="/disk2"/>
                                    </disk>
                                    <disk id="scratch" read_bw="100MBps" write_bw="100MBps">
                                       <prop id="size" value="101B"/>
                                       <prop id="mount" value="/scratch"/>
                                    </disk>
                                 </host>  
                                 <host id="QuadCoreHost" speed="1f" core="4" > 
                                    <disk id="large_disk" read_bw="100MBps" write_bw="100MBps">
                                       <prop id="size" value="100B"/>
                                       <prop id="mount" value="/"/>
                                    </disk>
                                    <disk id="scratch" read_bw="100MBps" write_bw="100MBps">
                                       <prop id="size" value="101B"/>
                                       <prop id="mount" value="/scratch"/>
                                    </disk>
                                 </host>  
                                 <link id="1" bandwidth="5000GBps" latency="0us"/>
                                 <route src="DualCoreHost" dst="QuadCoreHost"> <link_ctn id="1"/> </route>
                             </zone> 
                          </platform>)";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};

/**********************************************************************/
/**            MISSING ARGS TEST                                     **/
/**********************************************************************/

class MissingFailureCommandLineArgsTestWMS : public wrench::ExecutionController {

public:
    MissingFailureCommandLineArgsTestWMS(FailureCommandLineArgsTest *test,
                                         std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    FailureCommandLineArgsTest *test;

    int main() override {

        try {
            wrench::Simulation::turnOffHost("DualCoreHost");
            throw std::runtime_error("Shouldn't be able to turn off a host with passing a specific command-line arg");
        } catch (std::runtime_error &ignore) {
        }
        try {
            wrench::Simulation::turnOffLink("1");
            throw std::runtime_error("Shouldn't be able to turn off a host with passing a specific command-line arg");
        } catch (std::runtime_error &ignore) {
        }

        return 0;
    }
};

TEST_F(FailureCommandLineArgsTest, MissingFailureCommandLineArgsTest) {
    DO_TEST_WITH_FORK(do_MissingFailureCommandLineArgsTest_test);
}

void FailureCommandLineArgsTest::do_MissingFailureCommandLineArgsTest_test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string hostname = "DualCoreHost";
    ASSERT_NO_THROW(simulation->add(
            new MissingFailureCommandLineArgsTestWMS(this, hostname)));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
