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

// Convenient macros to launch a test inside a separate process
// and check the exit code, which denotes an error

#define DO_TEST_WITH_FORK(function){ \
                                      pid_t pid = fork(); \
                                      if (pid) { \
                                        int exit_code; \
                                        waitpid(pid, &exit_code, 0); \
                                        ASSERT_EQ(exit_code, 0); \
                                      } else { \
                                        this->function(); \
                                        exit((::testing::Test::HasFailure() ? 255 : 0)); \
                                      } \
                                   }

#define DO_TEST_WITH_FORK_EXPECT_FATAL_FAILURE(function, no_stderr){ \
                                      pid_t pid = fork(); \
                                      if (pid) { \
                                        int exit_code; \
                                        waitpid(pid, &exit_code, 0); \
                                        ASSERT_NE(WEXITSTATUS(exit_code), 255); \
                                        ASSERT_NE(WEXITSTATUS(exit_code), 0); \
                                        if (not no_stderr) { \
                                             std::cerr << "[ ** Observed a fatal failure (exit code: " + std::to_string(WEXITSTATUS(exit_code)) + "), as expected **]\n"; \
                                          } \
                                      } else { \
                                        if (no_stderr) { close(2); } \
                                        this->function(); \
                                        exit((::testing::Test::HasFailure() ? 255 : 0)); \
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
                                        exit((::testing::Test::HasFailure() ? 255 : 0)); \
                                      } \
                                   }

#endif //WRENCH_TESTWITHFORK_H_H
