/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>

#include <wrench/simulation/Simulation.h>
#include <wrench/data_file/DataFile.h>
#include <wrench/workflow/Workflow.h>

TEST(DataFileTest, FileStructure) {
    auto workflow = wrench::Workflow::createWorkflow();
    std::shared_ptr<wrench::DataFile> f1 = wrench::Simulation::addFile("file-01", 100);
    ASSERT_EQ(f1->getID(), "file-01");
    ASSERT_EQ(f1->getSize(), 100);

    auto size2 = wrench::Simulation::addFile("file-02", "100MB")->getSize();
    ASSERT_EQ(size2, 100000000ULL);
    auto size3 = wrench::Simulation::addFile("file-03", "100MiB")->getSize();
    ASSERT_EQ(size3, 100ULL * 1024ULL * 1024ULL);
    ASSERT_THROW(wrench::Simulation::addFile("file-04", "100MXiB"), std::invalid_argument);

    workflow->clear();
    wrench::Simulation::removeAllFiles();
}
