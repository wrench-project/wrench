/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <wrench/util/PointerUtil.h>

#include "../include/TestWithFork.h"

class PointerUtilTest : public ::testing::Test {
public:
    void do_moveUniqueFromSetToSet_test();
};

class TwoInts {
public:
    TwoInts(int x, int y) : x(x), y(y) {}

    int x;
    int y;
};

TEST_F(PointerUtilTest, MoveUniqueFromSetToSet) {
    DO_TEST_WITH_FORK(do_moveUniqueFromSetToSet_test);
}

void PointerUtilTest::do_moveUniqueFromSetToSet_test() {
    std::set<std::unique_ptr<TwoInts>> src_set;
    std::set<std::unique_ptr<TwoInts>> dst_set;

    src_set.insert(std::unique_ptr<TwoInts>(new TwoInts(2, 2)));
    src_set.insert(std::unique_ptr<TwoInts>(new TwoInts(2, 2)));

    dst_set.insert(std::unique_ptr<TwoInts>(new TwoInts(3, 3)));

    wrench::PointerUtil::moveUniquePtrFromSetToSet(src_set.begin(), &src_set, &dst_set);

    ASSERT_EQ(src_set.size(), 1);
    ASSERT_EQ(dst_set.size(), 2);


    int sum_src = 0;
    for (const auto &e: src_set) {
        sum_src += e->x + e->y;
    }
    int sum_dst = 0;
    for (const auto &e: dst_set) {
        sum_dst += e->x + e->y;
    }

    ASSERT_EQ(sum_src, 4);
    ASSERT_EQ(sum_dst, 10);
}
