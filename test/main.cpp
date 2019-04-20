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
//   xbt_log_control_set("root.thresh:critical");

//   xbt_log_control_set("simulation_timestamps.thresh:debug");

//   Example selective log enabling
   xbt_log_control_set("mailbox.thresh:debug");
//   xbt_log_control_set("cloud_service.thresh:info");
//   xbt_log_control_set("multicore_compute_service.thresh:info");
//   xbt_log_control_set("mailbox.thresh:debug");

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();

}
