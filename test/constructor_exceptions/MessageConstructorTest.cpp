/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>

#include "wrench/workflow/Workflow.h"
#include "../../src/wrench/services/file_registry/FileRegistryMessage.h"

class MessageConstructorTest : public ::testing::Test {
protected:
    MessageConstructorTest() {
      workflow = new wrench::Workflow();
      task = workflow->addTask("task", 1);
      file = workflow->addFile("file", 1);
    }

    // data members
    wrench::Workflow *workflow;
    wrench::WorkflowTask *task;
    wrench::WorkflowFile *file;
};


TEST_F(MessageConstructorTest, FileRegistryMessages) {
  EXPECT_NO_THROW(new wrench::FileRegistryFileLookupRequestMessage("mailbox", file, 666));
  EXPECT_THROW(new wrench::FileRegistryFileLookupRequestMessage("", file, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::FileRegistryFileLookupRequestMessage("mailbox", nullptr, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::FileRegistryFileLookupRequestMessage("mailbox", file, -1), std::invalid_argument);
}

