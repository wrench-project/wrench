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
#include "../../include/TestWithFork.h"


class InternalNetworkConnectionTest : public ::testing::Test {

public:


    void do_Constructor_test();


protected:
    InternalNetworkConnectionTest() {

        // Create a one-host platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"SingleHost\" speed=\"1f\"/> "
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

    }

    std::string platform_file_path = "/tmp/platform.xml";
};


/**********************************************************************/
/**  CONSTRUCTOR TEST                                                **/
/**********************************************************************/

TEST_F(InternalNetworkConnectionTest, Constructor) {
    DO_TEST_WITH_FORK(do_Constructor_test);
}

void InternalNetworkConnectionTest::do_Constructor_test() {

    wrench::NetworkConnection *conn = nullptr;
    auto *workflow = new wrench::Workflow();
    wrench::WorkflowFile *file = workflow->addFile("file", 10);
    wrench::StorageService *storage_service = (wrench::StorageService *) (1235);


    // Bogus type
    ASSERT_THROW(conn = new wrench::NetworkConnection(3, nullptr, storage_service, "/", storage_service, "/", "", ""), std::invalid_argument);
    ASSERT_THROW(conn = new wrench::NetworkConnection(4, nullptr, storage_service, "/", storage_service, "/", "", ""), std::invalid_argument);
    ASSERT_THROW(conn = new wrench::NetworkConnection(5, nullptr, storage_service, "/", storage_service, "/", "", ""), std::invalid_argument);
    ASSERT_THROW(conn = new wrench::NetworkConnection(6, nullptr, storage_service, "/", storage_service, "/", "", ""), std::invalid_argument);

    // Empty mailbox_name
    ASSERT_THROW(conn = new wrench::NetworkConnection(wrench::NetworkConnection::INCOMING_DATA, file, storage_service, "/", storage_service, "/", "", "ack"),
                 std::invalid_argument);

    // Empty file
    ASSERT_THROW(conn = new wrench::NetworkConnection(wrench::NetworkConnection::INCOMING_DATA, nullptr, storage_service, "/", storage_service, "/", "mailbox_name", "ack"),
                 std::invalid_argument);
    ASSERT_THROW(conn = new wrench::NetworkConnection(wrench::NetworkConnection::OUTGOING_DATA, nullptr, storage_service, "/", storage_service, "/", "mailbox_name", ""),
                 std::invalid_argument);

    // Ack mailbox_name should be empty
    ASSERT_THROW(conn = new wrench::NetworkConnection(wrench::NetworkConnection::OUTGOING_DATA, nullptr, storage_service, "/", storage_service, "/", "mailbox_name", "ack"),
                 std::invalid_argument);
    ASSERT_THROW(conn = new wrench::NetworkConnection(wrench::NetworkConnection::INCOMING_CONTROL, nullptr, storage_service, "/", storage_service, "/", "mailbox_name", "ack"),
                 std::invalid_argument);

    // Nullptr storage services
    ASSERT_THROW(conn = new wrench::NetworkConnection(wrench::NetworkConnection::OUTGOING_DATA, file, nullptr, "/", storage_service, "/", "mailbox_name", "ack"), std::invalid_argument);
    ASSERT_THROW(conn = new wrench::NetworkConnection(wrench::NetworkConnection::OUTGOING_DATA, file, storage_service, "/", nullptr, "/", "mailbox_name", "ack"), std::invalid_argument);
    //ASSERT_NO_THROW(conn = new wrench::NetworkConnection(wrench::NetworkConnection::INCOMING_CONTROL, nullptr, nullptr, "/", storage_service, "/", "mailbox_name", "ack"));
    delete conn;

    // Empty partitions
    ASSERT_THROW(conn = new wrench::NetworkConnection(wrench::NetworkConnection::OUTGOING_DATA, file, storage_service, "", storage_service, "/", "mailbox_name", "ack"), std::invalid_argument);
    ASSERT_THROW(conn = new wrench::NetworkConnection(wrench::NetworkConnection::OUTGOING_DATA, file, storage_service, "/", storage_service, "", "mailbox_name", "ack"), std::invalid_argument);

    // Non-Empty file
    ASSERT_THROW(conn = new wrench::NetworkConnection(wrench::NetworkConnection::INCOMING_CONTROL, file, nullptr, "/", nullptr, "/", "mailbox_name", ""), std::invalid_argument);

    ASSERT_NO_THROW(conn = new wrench::NetworkConnection(wrench::NetworkConnection::INCOMING_CONTROL, nullptr, storage_service, "/", storage_service, "/", "mailbox_name", ""));
    delete conn;
    ASSERT_NO_THROW(conn = new wrench::NetworkConnection(wrench::NetworkConnection::INCOMING_DATA, file, storage_service, "/", storage_service, "/", "mailbox_name", "ack"));
    delete conn;
    ASSERT_NO_THROW(conn = new wrench::NetworkConnection(wrench::NetworkConnection::OUTGOING_DATA, file, storage_service, "/", storage_service, "/", "mailbox_name", ""));
    delete conn;

    ASSERT_NO_THROW(conn = new wrench::NetworkConnection(wrench::NetworkConnection::OUTGOING_DATA, file, storage_service, "/", storage_service, "/", "mailbox_name", ""));
    ASSERT_THROW(conn->getMessage(), std::runtime_error);
    delete conn;

}
