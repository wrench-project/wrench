/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <gtest/gtest.h>
#include <map>
#include <wrench/services/compute/batch/BatchServiceMessage.h>

#include "wrench/workflow/Workflow.h"
#include "../../src/wrench/services/file_registry/FileRegistryMessage.h"
#include "wrench/services/compute/ComputeServiceMessage.h"
#include "../../src/wrench/services/storage/StorageServiceMessage.h"
#include "../../src/wrench/services/compute/virtualized_cluster/VirtualizedClusterServiceMessage.h"
#include "../../src/wrench/services/network_proximity/NetworkProximityMessage.h"
#include "wrench/workflow/execution_events/FailureCause.h"

class FailureCauseConstructorTest : public ::testing::Test {
protected:
    FailureCauseConstructorTest() {

    }

};



TEST_F(FailureCauseConstructorTest, NetworkError) {

  wrench::NetworkError *cause = nullptr;
  ASSERT_NO_THROW(cause = new wrench::NetworkError(wrench::NetworkError::SENDING, wrench::NetworkError::TIMEOUT, "mailbox"));
  ASSERT_NO_THROW(cause = new wrench::NetworkError(wrench::NetworkError::RECEIVING, wrench::NetworkError::TIMEOUT, "mailbox"));
  ASSERT_NO_THROW(cause = new wrench::NetworkError(wrench::NetworkError::SENDING, wrench::NetworkError::FAILURE, "mailbox"));
  ASSERT_NO_THROW(cause = new wrench::NetworkError(wrench::NetworkError::RECEIVING, wrench::NetworkError::FAILURE, "mailbox"));

  ASSERT_THROW(cause = new wrench::NetworkError(wrench::NetworkError::SENDING, wrench::NetworkError::TIMEOUT, ""), std::invalid_argument);

  ASSERT_NO_THROW(cause = new wrench::NetworkError(wrench::NetworkError::SENDING, wrench::NetworkError::TIMEOUT, "mailbox"));
  ASSERT_EQ(cause->isTimeout(), true);
  ASSERT_EQ(cause->whileReceiving(), false);
  ASSERT_EQ(cause->whileSending(), true);

  ASSERT_NO_THROW(cause = new wrench::NetworkError(wrench::NetworkError::RECEIVING, wrench::NetworkError::FAILURE, "mailbox"));
  ASSERT_EQ(cause->isTimeout(), false);
  ASSERT_EQ(cause->whileReceiving(), true);
  ASSERT_EQ(cause->whileSending(), false);


}
