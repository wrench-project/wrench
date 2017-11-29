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
      storage_service = (wrench::StorageService *)(1234);
    }

    // data members
    wrench::Workflow *workflow;
    wrench::WorkflowTask *task;
    wrench::WorkflowFile *file;
    wrench::StorageService *storage_service;
};



TEST_F(MessageConstructorTest, SimulationMessages) {

  EXPECT_NO_THROW(new wrench::SimulationMessage("name", 666));
  EXPECT_THROW(new wrench::SimulationMessage("", 666), std::invalid_argument);
  EXPECT_THROW(new wrench::SimulationMessage("name", -1), std::invalid_argument);

}


TEST_F(MessageConstructorTest, FileRegistryMessages) {

  EXPECT_NO_THROW(new wrench::FileRegistryFileLookupRequestMessage("mailbox", file, 666));
  EXPECT_THROW(new wrench::FileRegistryFileLookupRequestMessage("", file, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::FileRegistryFileLookupRequestMessage("mailbox", nullptr, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::FileRegistryFileLookupAnswerMessage(file, {}, 666));
  EXPECT_THROW(new wrench::FileRegistryFileLookupAnswerMessage(nullptr, {}, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::FileRegistryRemoveEntryRequestMessage("mailbox", file, storage_service, 666));
  EXPECT_THROW(new wrench::FileRegistryRemoveEntryRequestMessage("", file, storage_service, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::FileRegistryRemoveEntryRequestMessage("mailbox", nullptr, storage_service, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::FileRegistryRemoveEntryRequestMessage("mailbox", file, nullptr, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::FileRegistryRemoveEntryAnswerMessage(true, 666));

  EXPECT_NO_THROW(new wrench::FileRegistryAddEntryRequestMessage("mailbox", file, storage_service, 666));
  EXPECT_THROW(new wrench::FileRegistryAddEntryRequestMessage("", file, storage_service, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::FileRegistryAddEntryRequestMessage("mailbox", nullptr, storage_service, 666), std::invalid_argument);
  EXPECT_THROW(new wrench::FileRegistryAddEntryRequestMessage("mailbox", file, nullptr, 666), std::invalid_argument);

  EXPECT_NO_THROW(new wrench::FileRegistryAddEntryAnswerMessage(666));
}



