/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench-dev.h>
#include <gtest/gtest.h>
#include "../src/wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf/NodeAvailabilityTimeLine.h"
#include "../src/wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf_core_level/CoreAvailabilityTimeLine.h"

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(availability_timeline_test, "Log category for AvailabilityTimeLineTest");


class BatchServiceAvailabilityTimeLineTest : public ::testing::Test {

public:

    void do_NodeAvailabilityTimeLineTest_test();
    void do_CoreAvailabilityTimeLineTest_test();


protected:
    BatchServiceAvailabilityTimeLineTest() = default;

};


/**********************************************************************/
/**  NODE AVAILABILITY TIMELINE TEST                                 **/
/**********************************************************************/

TEST_F(BatchServiceAvailabilityTimeLineTest, NodeAvailabilityTimeLineTest)
{
    DO_TEST_WITH_FORK(do_NodeAvailabilityTimeLineTest_test);
}

void BatchServiceAvailabilityTimeLineTest::do_NodeAvailabilityTimeLineTest_test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
//    argv[1] = strdup("--wrench-full-log");

    auto tl = new wrench::NodeAvailabilityTimeLine(10);

    auto wj1 = std::shared_ptr<wrench::CompoundJob>((wrench::CompoundJob *)(1234), [](void *ptr){});

    auto bj1 = std::shared_ptr<wrench::BatchJob>(new wrench::BatchJob(wj1, 1, 10, 5, 1, "who", 0, 0));

    tl->add(0, 10.0, bj1);

    // Note that there is check on the validity of the interval in terms of the number of nodes
    auto wj2 = std::shared_ptr<wrench::CompoundJob>((wrench::CompoundJob *)(1234), [](void *ptr){});
    auto bj2 = std::shared_ptr<wrench::BatchJob>(new wrench::BatchJob(wj2, 2, 20, 20, 1, "who", 0, 0));
    tl->add(10.0, 30.0, bj2);

    // Can even have weird nonsensical start and end...
    auto wj3 = std::shared_ptr<wrench::CompoundJob>((wrench::CompoundJob *)(1234), [](void *ptr){});
    auto bj3 = std::shared_ptr<wrench::BatchJob>(new wrench::BatchJob(wj2, 3, 20, 20, 1, "who", 0, 0));
    tl->add(500.0, 30.0, bj3);

    tl->print();
    tl->clear();

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  CORE AVAILABILITY TIMELINE TEST                                 **/
/**********************************************************************/

TEST_F(BatchServiceAvailabilityTimeLineTest, CoreAvailabilityTimeLineTest)
{
    DO_TEST_WITH_FORK(do_CoreAvailabilityTimeLineTest_test);
}

void BatchServiceAvailabilityTimeLineTest::do_CoreAvailabilityTimeLineTest_test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
//    argv[1] = strdup("--wrench-full-log");

    auto tl = new wrench::CoreAvailabilityTimeLine(10, 6);

    auto wj1 = std::shared_ptr<wrench::CompoundJob>((wrench::CompoundJob *)(1234), [](void *ptr){});

    auto bj1 = std::shared_ptr<wrench::BatchJob>(new wrench::BatchJob(wj1, 1, 10, 5, 1, "who", 0, 0));

    tl->add(0, 10.0, bj1);

    // Note that there is check on the validity of the interval in terms of the number of nodes
    auto wj2 = std::shared_ptr<wrench::CompoundJob>((wrench::CompoundJob *)(1234), [](void *ptr){});
    auto bj2 = std::shared_ptr<wrench::BatchJob>(new wrench::BatchJob(wj2, 2, 20, 20, 1, "who", 0, 0));
    tl->add(10.0, 30.0, bj2);

    // Can even have weird nonsensical start and end...
    auto wj3 = std::shared_ptr<wrench::CompoundJob>((wrench::CompoundJob *)(1234), [](void *ptr){});
    auto bj3 = std::shared_ptr<wrench::BatchJob>(new wrench::BatchJob(wj2, 3, 20, 20, 1, "who", 0, 0));
    tl->add(500.0, 30.0, bj3);

    tl->print();
    tl->clear();

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
