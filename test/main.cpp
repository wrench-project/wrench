/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <xbt.h>

int main(int argc, char **argv) {
  // disable log
  xbt_log_control_set("root.thresh:critical");

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
