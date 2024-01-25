/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_RUNTIMEASSERT_H
#define WRENCH_RUNTIMEASSERT_H

// Convenient macros to throw a runtime exception

#define RUNTIME_DBL_EQ(value, expected, message, epsilon)                                                \
    {                                                                                                    \
        if (((value) < (expected) - (epsilon)) or ((value) > (expected) + (epsilon))) {                  \
            std::stringstream ss;                                                                        \
            ss << "Unexpected " << (message) << ": " << (value) << " (expected: ~" << (expected) << ")"; \
            throw std::runtime_error(ss.str());                                                          \
        }                                                                                                \
    }

#define RUNTIME_EQ(value, expected, message)                                                            \
    {                                                                                                   \
        if ((value) != (expected)) {                                                                    \
            std::stringstream ss;                                                                       \
            ss << "Unexpected " << (message) << ": " << (value) << " (expected: " << (expected) << ")"; \
            throw std::runtime_error(ss.str());                                                         \
        }                                                                                               \
    }


#endif//WRENCH_RUNTIMEASSERT_H
