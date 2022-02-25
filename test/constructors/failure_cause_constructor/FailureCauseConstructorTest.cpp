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

#include <wrench/failure_causes/HostError.h>
#include <wrench/failure_causes/NetworkError.h>
#include <wrench/failure_causes/ComputeThreadHasDied.h>
#include <wrench/failure_causes/FatalFailure.h>

class FailureCauseConstructorTest : public ::testing::Test {
protected:
    FailureCauseConstructorTest() {
    }
};

TEST_F(FailureCauseConstructorTest, NetworkError) {

  wrench::NetworkError *cause;
  ASSERT_NO_THROW(cause = new wrench::NetworkError(wrench::NetworkError::SENDING, wrench::NetworkError::TIMEOUT, "mailbox"));
  ASSERT_NO_THROW(cause = new wrench::NetworkError(wrench::NetworkError::RECEIVING, wrench::NetworkError::TIMEOUT, "mailbox"));
  ASSERT_NO_THROW(cause = new wrench::NetworkError(wrench::NetworkError::SENDING, wrench::NetworkError::FAILURE, "mailbox"));
  ASSERT_NO_THROW(cause = new wrench::NetworkError(wrench::NetworkError::RECEIVING, wrench::NetworkError::FAILURE, "mailbox"));
  ASSERT_NO_THROW(cause = new wrench::NetworkError(wrench::NetworkError::RECEIVING, wrench::NetworkError::FAILURE, "mailbox"));
  ASSERT_THROW(cause = new wrench::NetworkError(wrench::NetworkError::RECEIVING, wrench::NetworkError::FAILURE, ""), std::invalid_argument);


  ASSERT_NO_THROW(cause = new wrench::NetworkError(wrench::NetworkError::SENDING, wrench::NetworkError::TIMEOUT, "mailbox"));
  ASSERT_EQ(cause->isTimeout(), true);
  ASSERT_EQ(cause->whileReceiving(), false);
  ASSERT_EQ(cause->whileSending(), true);

  ASSERT_NO_THROW(cause = new wrench::NetworkError(wrench::NetworkError::RECEIVING, wrench::NetworkError::FAILURE, "mailbox"));
  ASSERT_EQ(cause->isTimeout(), false);
  ASSERT_EQ(cause->whileReceiving(), true);
  ASSERT_EQ(cause->whileSending(), false);

}

TEST_F(FailureCauseConstructorTest, ComputeThreadHasDied) {

    wrench::ComputeThreadHasDied *cause = nullptr;
    ASSERT_NO_THROW(cause = new wrench::ComputeThreadHasDied());
    if (cause) {
      cause->toString(); // Coverage
    }
}

TEST_F(FailureCauseConstructorTest, FatalFailure) {

    wrench::FatalFailure *cause = nullptr;
    ASSERT_NO_THROW(cause = new wrench::FatalFailure(""));
    if (cause) {
      cause->toString(); // Coverage
    }
}


TEST_F(FailureCauseConstructorTest, HostError) {
    wrench::HostError *cause = nullptr;
    ASSERT_NO_THROW(cause = new wrench::HostError("hostname"));
    if (cause) {
      cause->toString(); // Coverage
    }
}
