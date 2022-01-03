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

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(bad_platform_test, "Log category for BadPlatform test");


class BadPlatformTest : public ::testing::Test {

public:

    void do_badPlatformFileTest_test();
    void do_badPlatformTest_test(std::string xml);

protected:


    std::string bad_format_xml = "<?xml version='1.0'?>"
                               "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                               "<platform version=\"4.1\"> "
                               " < "
                               "   <zone id=\"AS0\" routing=\"Full\"> "
                               "       <host id=\"Host\" speed=\"1f\" core=\"2\" /> "
                               "   </zone> "
                               "</platform>";

    std::string bad_ram1_xml = "<?xml version='1.0'?>"
                               "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                               "<platform version=\"4.1\"> "
                               "   <zone id=\"AS0\" routing=\"Full\"> "
                               "       <host id=\"Host\" speed=\"1f\" core=\"2\" > "
                               "              <prop id=\"RAM\" value=\"bogus\"/>"
                               "       </host>  "
                               "   </zone> "
                               "</platform>";

    std::string bad_ram2_xml = "<?xml version='1.0'?>"
                               "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                               "<platform version=\"4.1\"> "
                               "   <zone id=\"AS01\" routing=\"Full\"> "
                               "       <host id=\"Host\" speed=\"1f\" core=\"2\" > "
                               "              <prop id=\"RAM\" value=\"100Y\"/>"
                               "       </host>  "
                               "   </zone> "
                               "</platform>";


    std::string bad_disk1_xml = "<?xml version='1.0'?>"
                                "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                                "<platform version=\"4.1\"> "
                                "   <zone id=\"AS01\" routing=\"Full\"> "
                                "       <host id=\"Host\" speed=\"1f\" core=\"2\" > "
                                "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                                "             <prop id=\"size\" value=\"100B\"/>"
                                "             <prop id=\"mount\" value=\"/disk1 bogus\"/>"
                                "          </disk>"
                                "       </host>  "
                                "   </zone> "
                                "</platform>";

    std::string bad_disk2_xml = "<?xml version='1.0'?>"
                                "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                                "<platform version=\"4.1\"> "
                                "   <zone id=\"AS01\" routing=\"Full\"> "
                                "       <host id=\"Host\" speed=\"1f\" core=\"2\" > "
                                "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                                "             <prop id=\"size\" value=\"100B\"/>"
                                "             <prop id=\"mount\" value=\"/disk1\"/>"
                                "          </disk>"
                                "          <disk id=\"large_disk2\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                                "             <prop id=\"size\" value=\"100B\"/>"
                                "             <prop id=\"mount\" value=\"/disk1\"/>"
                                "          </disk>"
                                "       </host>  "
                                "   </zone> "
                                "</platform>";

    std::string bad_disk3_xml = "<?xml version='1.0'?>"
                                "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                                "<platform version=\"4.1\"> "
                                "   <zone id=\"AS01\" routing=\"Full\"> "
                                "       <host id=\"Host\" speed=\"1f\" core=\"2\" > "
                                "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                                "             <prop id=\"size\" value=\"100B\"/>"
                                "             <prop id=\"mount\" value=\"/disk1\"/>"
                                "          </disk>"
                                "          <disk id=\"large_disk2\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                                "             <prop id=\"size\" value=\"100B\"/>"
                                "             <prop id=\"mount\" value=\"/disk1/foo\"/>"
                                "          </disk>"
                                "       </host>  "
                                "   </zone> "
                                "</platform>";

    std::string zero_bandwidth_link = "<?xml version='1.0'?>"
                                  "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                                  "<platform version=\"4.1\"> "
                                  "   <zone id=\"AS01\" routing=\"Full\"> "
                                  "       <link id=\"link\" bandwidth=\"0Mbps\" latency=\"0us\" > "
                                  "       </link>  "
                                  "   </zone> "
                                  "</platform>";

    std::string zero_speed_host = "<?xml version='1.0'?>"
                                "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                                "<platform version=\"4.1\"> "
                                "   <zone id=\"AS01\" routing=\"Full\"> "
                                "       <host id=\"Host\" speed=\"0f\" core=\"2\" > "
                                "       </host>  "
                                "   </zone> "
                                "</platform>";



    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};

/**********************************************************************/
/**                   BAD PLATFORM FILE TEST                         **/
/**********************************************************************/


TEST_F(BadPlatformTest, BadPlatformFile) {
        DO_TEST_WITH_FORK(do_badPlatformFileTest_test);
}

void BadPlatformTest::do_badPlatformFileTest_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    ASSERT_THROW(simulation->instantiatePlatform("/bogus"), std::invalid_argument);

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**                        BAD PLATFORM TEST                         **/
/**********************************************************************/


TEST_F(BadPlatformTest, BadPlatform) {
    auto bad_xmls = {
            this->bad_format_xml,
            this->bad_ram1_xml,
            this->bad_ram2_xml,
            this->bad_disk1_xml,
            this->bad_disk2_xml,
            this->bad_disk3_xml,
            this->zero_bandwidth_link
    };
    for (auto const &xml : bad_xmls) {
        DO_TEST_WITH_FORK_ONE_ARG(do_badPlatformTest_test, xml);
    }
}

void BadPlatformTest::do_badPlatformTest_test(std::string xml) {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    FILE *platform_file = fopen(platform_file_path.c_str(), "w");
    fprintf(platform_file, "%s", xml.c_str());
    fclose(platform_file);
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::invalid_argument);

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


