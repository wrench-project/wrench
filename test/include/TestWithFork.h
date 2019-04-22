/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_TESTWITHFORK_H_H
#define WRENCH_TESTWITHFORK_H_H

#include <stdlib.h>

// Convenient macro to launch a test inside a separate process
// and check the exit code, which denotes an error
#define DO_TEST_WITH_FORK(function){ \
                                      pid_t pid = fork(); \
                                      if (pid) { \
                                        int exit_code; \
                                        waitpid(pid, &exit_code, 0); \
                                        ASSERT_EQ(exit_code, 0); \
                                      } else { \
                                        this->function(); \
                                        exit((::testing::Test::HasFailure() ? 666 : 0)); \
                                      } \
                                   }

#define DO_TEST_WITH_FORK_ONE_ARG(function, arg){ \
                                      pid_t pid = fork(); \
                                      if (pid) { \
                                        int exit_code; \
                                        waitpid(pid, &exit_code, 0); \
                                        ASSERT_EQ(exit_code, 0); \
                                      } else { \
                                        this->function(arg); \
                                        exit((::testing::Test::HasFailure() ? 666 : 0)); \
                                      } \
                                   }

#endif //WRENCH_TESTWITHFORK_H_H
