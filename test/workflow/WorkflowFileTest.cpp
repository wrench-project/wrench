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

TEST(WorkflowFileTest, FileStructure) {
  wrench::Workflow workflow;
  wrench::WorkflowFile *f1 = workflow.addFile("file-01", 100);

  ASSERT_EQ(f1->getID(), "file-01");
  ASSERT_EQ(f1->getSize(), 100);
}
