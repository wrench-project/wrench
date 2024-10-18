/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <wrench/util/UnitParser.h>
#include "../include/TestWithFork.h"


class UnitParserTest : public ::testing::Test {
public:
    void do_parse_test();
};


TEST_F(UnitParserTest, ParseTest) {
    DO_TEST_WITH_FORK(do_parse_test);
}

void UnitParserTest::do_parse_test() {

    ASSERT_DOUBLE_EQ(wrench::UnitParser::parse_size("10"), 10.0);
    ASSERT_DOUBLE_EQ(wrench::UnitParser::parse_size("10B"), 10.0);
    ASSERT_EQ(wrench::UnitParser::parse_size("10kB"), 10 * 1000);
    ASSERT_EQ(wrench::UnitParser::parse_size("10MB"), 10 * 1000 * 1000ULL);
    ASSERT_EQ(wrench::UnitParser::parse_size("10GB"), 10000000000ULL);
    ASSERT_THROW(wrench::UnitParser::parse_size("10FO"), std::invalid_argument);

    ASSERT_DOUBLE_EQ(wrench::UnitParser::parse_compute_speed("10"), 10.0);
    ASSERT_DOUBLE_EQ(wrench::UnitParser::parse_compute_speed("10f"), 10.0);
    ASSERT_DOUBLE_EQ(wrench::UnitParser::parse_compute_speed("10kf"), 10.0 * 1000.0);
    ASSERT_DOUBLE_EQ(wrench::UnitParser::parse_compute_speed("10Mf"), 10.0 * 1000.0 * 1000.0);
    ASSERT_THROW(wrench::UnitParser::parse_compute_speed("10FO"), std::invalid_argument);

    ASSERT_DOUBLE_EQ(wrench::UnitParser::parse_time("10s"), 10.0);
    ASSERT_THROW(wrench::UnitParser::parse_time("10FO"), std::invalid_argument);

    ASSERT_DOUBLE_EQ(wrench::UnitParser::parse_bandwidth("10Bps"), 10.0);
    ASSERT_THROW(wrench::UnitParser::parse_bandwidth("10FO"), std::invalid_argument);
}
